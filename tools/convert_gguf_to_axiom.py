#!/usr/bin/env python3
# Project Axiom — Built from scratch. Engineered for the future.
# Copyright (c) 2026 Omid Teimory. All rights reserved.
"""
Project Axiom — High-Precision GGUF to .axiom Converter & Local Model Auditor
File: tools/convert_gguf_to_axiom.py

Features:
1. Scans local Hugging Face cache & Ollama manifests/blobs to resolve GGUF paths automatically.
2. Parses GGUF v2/v3 headers, metadata KV pairs, and tensor offset maps via zero-copy mmap.
3. Dequantizes F32, F16, BF16, Q8_0, Q4_0, Q4_1, Q4_K, Q5_K, and Q6_K tensors in pure vectorized NumPy
   with full FP32 intermediate accumulation.
4. Converts FP32 -> BF16 using IEEE-754 Round-to-Nearest-Even (RNE) with NaN preservation
   (eliminating the -0.5 ULP systematic bias of naive `uint32 >> 16` truncation).
5. Streams contiguous BF16 weights directly into `<output>.axiom` (64-byte `<10I2f16x` header)
   and exports decoded UTF-8 tokens into `<output>_vocab.axiom`.
"""

from __future__ import annotations

import argparse
import json
import mmap
import os
import struct
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, BinaryIO, Dict, List, Optional, Tuple

import numpy as np

# ============================================================================
# 1. CONSTANTS & AXIOM BINARY SPECIFICATION
# ============================================================================

AXIOM_MAGIC = 0x4D4F4958        # ASCII "XIOM" in little-endian
AXIOM_VERSION = 2
AXIOM_HEADER_STRUCT = struct.Struct("<10I2f16x")  # Strictly 64 bytes

GGUF_MAGIC = 0x46554747         # ASCII "GGUF" in little-endian
GGUF_DEFAULT_ALIGNMENT = 32

# GGML Tensor Type IDs
GGML_TYPE_F32 = 0
GGML_TYPE_F16 = 1
GGML_TYPE_Q4_0 = 2
GGML_TYPE_Q4_1 = 3
GGML_TYPE_Q8_0 = 8
GGML_TYPE_Q4_K = 12
GGML_TYPE_Q5_K = 13
GGML_TYPE_Q6_K = 14
GGML_TYPE_BF16 = 30

# Block sizes (elements per block, bytes per block)
GGML_BLOCK_INFO: Dict[int, Tuple[int, int, str]] = {
    GGML_TYPE_F32:  (1,   4,   "F32"),
    GGML_TYPE_F16:  (1,   2,   "F16"),
    GGML_TYPE_BF16: (1,   2,   "BF16"),
    GGML_TYPE_Q4_0: (32,  18,  "Q4_0"),
    GGML_TYPE_Q4_1: (32,  20,  "Q4_1"),
    GGML_TYPE_Q8_0: (32,  34,  "Q8_0"),
    GGML_TYPE_Q4_K: (256, 144, "Q4_K"),
    GGML_TYPE_Q5_K: (256, 176, "Q5_K"),
    GGML_TYPE_Q6_K: (256, 210, "Q6_K"),
}

# GGUF Metadata Value Types
GGUF_TYPE_UINT8 = 0
GGUF_TYPE_INT8 = 1
GGUF_TYPE_UINT16 = 2
GGUF_TYPE_INT16 = 3
GGUF_TYPE_UINT32 = 4
GGUF_TYPE_INT32 = 5
GGUF_TYPE_FLOAT32 = 6
GGUF_TYPE_BOOL = 7
GGUF_TYPE_STRING = 8
GGUF_TYPE_ARRAY = 9
GGUF_TYPE_UINT64 = 10
GGUF_TYPE_INT64 = 11
GGUF_TYPE_FLOAT64 = 12

SCALAR_STRUCTS: Dict[int, struct.Struct] = {
    GGUF_TYPE_UINT8:   struct.Struct("<B"),
    GGUF_TYPE_INT8:    struct.Struct("<b"),
    GGUF_TYPE_UINT16:  struct.Struct("<H"),
    GGUF_TYPE_INT16:   struct.Struct("<h"),
    GGUF_TYPE_UINT32:  struct.Struct("<I"),
    GGUF_TYPE_INT32:   struct.Struct("<i"),
    GGUF_TYPE_FLOAT32: struct.Struct("<f"),
    GGUF_TYPE_BOOL:    struct.Struct("<?"),
    GGUF_TYPE_UINT64:  struct.Struct("<Q"),
    GGUF_TYPE_INT64:   struct.Struct("<q"),
    GGUF_TYPE_FLOAT64: struct.Struct("<d"),
}


@dataclass
class AxiomModelConfig:
    num_layers: int
    hidden_dim: int
    num_q_heads: int
    num_kv_heads: int
    head_dim: int
    feedforward_network_dimension: int
    vocab_size: int
    max_seq_len: int
    rms_norm_eps: float
    rope_theta: float

    def pack(self) -> bytes:
        packed = AXIOM_HEADER_STRUCT.pack(
            AXIOM_MAGIC,
            AXIOM_VERSION,
            self.num_layers,
            self.hidden_dim,
            self.num_q_heads,
            self.num_kv_heads,
            self.head_dim,
            self.feedforward_network_dimension,
            self.vocab_size,
            self.max_seq_len,
            float(self.rms_norm_eps),
            float(self.rope_theta),
        )
        assert len(packed) == 64, f"Header size mismatch: {len(packed)} != 64"
        return packed


@dataclass
class GGUFTensorInfo:
    name: str
    dims: Tuple[int, ...]
    ggml_type: int
    offset: int

    @property
    def num_elements(self) -> int:
        prod = 1
        for d in self.dims:
            prod *= d
        return prod

    @property
    def byte_size(self) -> int:
        if self.ggml_type not in GGML_BLOCK_INFO:
            raise ValueError(f"Unsupported GGML tensor type ID: {self.ggml_type} for {self.name}")
        block_elems, block_bytes, _ = GGML_BLOCK_INFO[self.ggml_type]
        if self.num_elements % block_elems != 0:
            raise ValueError(
                f"Tensor {self.name} element count ({self.num_elements}) "
                f"not divisible by block size ({block_elems})"
            )
        return (self.num_elements // block_elems) * block_bytes

    @property
    def type_name(self) -> str:
        return GGML_BLOCK_INFO.get(self.ggml_type, (0, 0, f"UNKNOWN({self.ggml_type})"))[2]


# ============================================================================
# 2. PRECISION-PRESERVING DEQUANTIZATION & IEEE-754 RNE BF16 CONVERSION
# ============================================================================

def fp32_to_bf16_rne(arr_f32: np.ndarray) -> np.ndarray:
    """
    Converts a float32 NumPy array to raw uint16 (bfloat16 bit pattern) using
    IEEE-754 Round-to-Nearest-Even (RNE) with exact NaN preservation.

    Why not `u32 >> 16`?
    Naive truncation rounds toward zero, introducing a -0.5 ULP drift across
    weights that compounds across 28+ transformer layers. RNE adds the rounding
    bias `0x7FFF + ((u32 >> 16) & 1)` prior to shifting.
    """
    f32 = np.ascontiguousarray(arr_f32, dtype=np.float32)
    u32 = f32.view(np.uint32)

    # Identify NaNs (exponent == 0xFF and mantissa != 0)
    is_nan = (u32 & 0x7FFFFFFF) > 0x7F800000

    # Round-to-Nearest-Even bias: 0x7FFF + LSB of target BF16 word
    lsb = (u32 >> 16) & np.uint32(1)
    rounding_bias = np.uint32(0x00007FFF) + lsb
    rounded = u32 + rounding_bias

    bf16 = (rounded >> 16).astype(np.uint16)

    # Force quiet NaN (0x7FC0) where input was NaN so payload overflow doesn't turn NaN into Inf
    if np.any(is_nan):
        bf16 = np.where(is_nan, np.uint16(0x7FC0), bf16)

    return bf16


def dequantize_to_bf16(raw_bytes: memoryview, ggml_type: int, num_elements: int) -> np.ndarray:
    """
    Unpacks a GGML tensor buffer into float32 (preserving full block scale & min precision)
    and converts to BF16 using IEEE-754 RNE. Fast-paths native BF16.
    """
    if ggml_type == GGML_TYPE_BF16:
        # Already native BF16 — zero precision loss copy
        return np.frombuffer(raw_bytes, dtype=np.uint16, count=num_elements)

    if ggml_type == GGML_TYPE_F32:
        f32 = np.frombuffer(raw_bytes, dtype=np.float32, count=num_elements)
        return fp32_to_bf16_rne(f32)

    if ggml_type == GGML_TYPE_F16:
        f16 = np.frombuffer(raw_bytes, dtype=np.float16, count=num_elements)
        return fp32_to_bf16_rne(f16.astype(np.float32))

    if ggml_type == GGML_TYPE_Q8_0:
        # 32 elements per 34-byte block: [fp16 d (2B), int8 qs[32] (32B)]
        num_blocks = num_elements // 32
        blocks = np.frombuffer(raw_bytes, dtype=np.uint8, count=num_blocks * 34).reshape(num_blocks, 34)
        d = blocks[:, :2].copy().view(np.float16).astype(np.float32)  # (N, 1)
        qs = blocks[:, 2:].view(np.int8).astype(np.float32)           # (N, 32)
        f32 = (d * qs).reshape(-1)
        return fp32_to_bf16_rne(f32)

    if ggml_type == GGML_TYPE_Q4_0:
        # 32 elements per 18-byte block: [fp16 d (2B), uint8 nibbles[16] (16B)]
        num_blocks = num_elements // 32
        blocks = np.frombuffer(raw_bytes, dtype=np.uint8, count=num_blocks * 18).reshape(num_blocks, 18)
        d = blocks[:, :2].copy().view(np.float16).astype(np.float32)
        qs = blocks[:, 2:]
        lo = (qs & 0x0F).astype(np.int8) - 8
        hi = (qs >> 4).astype(np.int8) - 8
        weights = np.concatenate([lo, hi], axis=1).astype(np.float32)
        f32 = (d * weights).reshape(-1)
        return fp32_to_bf16_rne(f32)

    if ggml_type == GGML_TYPE_Q4_1:
        # 32 elements per 20-byte block: [fp16 d (2B), fp16 m (2B), uint8 nibbles[16] (16B)]
        num_blocks = num_elements // 32
        blocks = np.frombuffer(raw_bytes, dtype=np.uint8, count=num_blocks * 20).reshape(num_blocks, 20)
        d = blocks[:, 0:2].copy().view(np.float16).astype(np.float32)
        m = blocks[:, 2:4].copy().view(np.float16).astype(np.float32)
        qs = blocks[:, 4:]
        lo = (qs & 0x0F).astype(np.float32)
        hi = (qs >> 4).astype(np.float32)
        weights = np.concatenate([lo, hi], axis=1)
        f32 = (d * weights + m).reshape(-1)
        return fp32_to_bf16_rne(f32)

    if ggml_type == GGML_TYPE_Q4_K:
        return _dequantize_q4_k_to_bf16(raw_bytes, num_elements)

    if ggml_type == GGML_TYPE_Q5_K:
        return _dequantize_q5_k_to_bf16(raw_bytes, num_elements)

    if ggml_type == GGML_TYPE_Q6_K:
        return _dequantize_q6_k_to_bf16(raw_bytes, num_elements)

    raise NotImplementedError(f"GGML quantization type {ggml_type} is not supported.")


def _unpack_k_scales(scales: np.ndarray) -> Tuple[np.ndarray, np.ndarray]:
    """
    Unpacks the 12-byte packed 6-bit scales & mins array used in Q4_K and Q5_K.
    Input shape: (num_blocks, 12) uint8
    Output: sc (num_blocks, 8) float32, m (num_blocks, 8) float32
    Matches ggml `get_scale_min_k4` exact bit layout.
    """
    sc = np.empty((scales.shape[0], 8), dtype=np.uint8)
    m = np.empty((scales.shape[0], 8), dtype=np.uint8)

    # j = 0..3: lower 6 bits of bytes 0..3 and 4..7
    sc[:, 0:4] = scales[:, 0:4] & 63
    m[:, 0:4] = scales[:, 4:8] & 63

    # j = 4..7: lower 4 bits of bytes 8..11 combined with top 2 bits of bytes 0..7
    sc[:, 4:8] = (scales[:, 8:12] & 0x0F) | ((scales[:, 0:4] >> 6) << 4)
    m[:, 4:8] = (scales[:, 8:12] >> 4) | ((scales[:, 4:8] >> 6) << 4)

    return sc.astype(np.float32), m.astype(np.float32)


def _dequantize_q4_k_to_bf16(raw_bytes: memoryview, num_elements: int) -> np.ndarray:
    """
    Vectorized Q4_K dequantizer (256 elements per 144-byte super-block).
    Layout:
      - d: fp16 (2B)
      - dmin: fp16 (2B)
      - scales: uint8[12] (12B) -> 8 scales & 8 mins (6-bit each)
      - qs: uint8[128] (128B) -> 256 4-bit weights in 4 chunks of 64
    """
    num_blocks = num_elements // 256
    blocks = np.frombuffer(raw_bytes, dtype=np.uint8, count=num_blocks * 144).reshape(num_blocks, 144)

    d = blocks[:, 0:2].copy().view(np.float16).astype(np.float32)     # (N, 1)
    dmin = blocks[:, 2:4].copy().view(np.float16).astype(np.float32)  # (N, 1)
    sc, m = _unpack_k_scales(blocks[:, 4:16])                         # (N, 8), (N, 8)
    qs = blocks[:, 16:144].reshape(num_blocks, 4, 32)                 # (N, 4, 32)

    out = np.empty((num_blocks, 8, 32), dtype=np.float32)
    for chunk in range(4):
        q_chunk = qs[:, chunk, :]
        q_lo = (q_chunk & 0x0F).astype(np.float32)
        q_hi = (q_chunk >> 4).astype(np.float32)

        s0 = (d[:, 0] * sc[:, 2 * chunk])[:, None]
        m0 = (dmin[:, 0] * m[:, 2 * chunk])[:, None]
        s1 = (d[:, 0] * sc[:, 2 * chunk + 1])[:, None]
        m1 = (dmin[:, 0] * m[:, 2 * chunk + 1])[:, None]

        out[:, 2 * chunk, :] = s0 * q_lo - m0
        out[:, 2 * chunk + 1, :] = s1 * q_hi - m1

    return fp32_to_bf16_rne(out.reshape(-1))


def _dequantize_q5_k_to_bf16(raw_bytes: memoryview, num_elements: int) -> np.ndarray:
    """
    Vectorized Q5_K dequantizer (256 elements per 176-byte super-block).
    Layout:
      - d: fp16 (2B), dmin: fp16 (2B), scales: uint8[12] (12B)
      - qh: uint8[32] (32B) -> high 5th bit for 256 weights
      - qs: uint8[128] (128B) -> low 4 bits for 256 weights
    """
    num_blocks = num_elements // 256
    blocks = np.frombuffer(raw_bytes, dtype=np.uint8, count=num_blocks * 176).reshape(num_blocks, 176)

    d = blocks[:, 0:2].copy().view(np.float16).astype(np.float32)
    dmin = blocks[:, 2:4].copy().view(np.float16).astype(np.float32)
    sc, m = _unpack_k_scales(blocks[:, 4:16])
    qh = blocks[:, 16:48]                                             # (N, 32)
    qs = blocks[:, 48:176].reshape(num_blocks, 4, 32)                 # (N, 4, 32)

    out = np.empty((num_blocks, 8, 32), dtype=np.float32)
    for chunk in range(4):
        q_chunk = qs[:, chunk, :]
        hb_lo = ((qh >> (2 * chunk)) & 1) << 4
        hb_hi = ((qh >> (2 * chunk + 1)) & 1) << 4

        q_lo = ((q_chunk & 0x0F) | hb_lo).astype(np.float32)
        q_hi = ((q_chunk >> 4) | hb_hi).astype(np.float32)

        s0 = (d[:, 0] * sc[:, 2 * chunk])[:, None]
        m0 = (dmin[:, 0] * m[:, 2 * chunk])[:, None]
        s1 = (d[:, 0] * sc[:, 2 * chunk + 1])[:, None]
        m1 = (dmin[:, 0] * m[:, 2 * chunk + 1])[:, None]

        out[:, 2 * chunk, :] = s0 * q_lo - m0
        out[:, 2 * chunk + 1, :] = s1 * q_hi - m1

    return fp32_to_bf16_rne(out.reshape(-1))


def _dequantize_q6_k_to_bf16(raw_bytes: memoryview, num_elements: int) -> np.ndarray:
    """
    Vectorized Q6_K dequantizer (256 elements per 210-byte super-block).
    Critical for Q4_K_M models because Qwen token_embd, output, attn_v, and ffn_down
    tensors are frequently stored in Q6_K for quality preservation.
    Layout:
      - ql: uint8[128] (low 4 bits)
      - qh: uint8[64]  (high 2 bits)
      - scales: int8[16] (sub-block scales for 16 groups of 16 weights)
      - d: fp16 (2B super-block scale at bytes 208..210)
    """
    num_blocks = num_elements // 256
    blocks = np.frombuffer(raw_bytes, dtype=np.uint8, count=num_blocks * 210).reshape(num_blocks, 210)

    ql = blocks[:, 0:128].reshape(num_blocks, 2, 64)
    qh = blocks[:, 128:192].reshape(num_blocks, 2, 32)
    scales = blocks[:, 192:208].view(np.int8).astype(np.float32).reshape(num_blocks, 2, 8)
    d = blocks[:, 208:210].copy().view(np.float16).astype(np.float32)  # (N, 1)

    out = np.empty((num_blocks, 2, 4, 32), dtype=np.float32)
    for half in range(2):
        ql_h = ql[:, half, :]   # (N, 64)
        qh_h = qh[:, half, :]   # (N, 32)
        sc_h = scales[:, half, :]  # (N, 8)

        q1 = ((ql_h[:, 0:32] & 0x0F) | (((qh_h >> 0) & 3) << 4)).astype(np.int8) - 32
        q2 = ((ql_h[:, 32:64] & 0x0F) | (((qh_h >> 2) & 3) << 4)).astype(np.int8) - 32
        q3 = ((ql_h[:, 0:32] >> 4) | (((qh_h >> 4) & 3) << 4)).astype(np.int8) - 32
        q4 = ((ql_h[:, 32:64] >> 4) | (((qh_h >> 6) & 3) << 4)).astype(np.int8) - 32

        for idx, q_group in enumerate((q1, q2, q3, q4)):
            # First 16 elements use scale[2*idx], next 16 use scale[2*idx + 1]
            s_left = (d[:, 0] * sc_h[:, 2 * idx])[:, None]
            s_right = (d[:, 0] * sc_h[:, 2 * idx + 1])[:, None]
            out[:, half, idx, 0:16] = s_left * q_group[:, 0:16].astype(np.float32)
            out[:, half, idx, 16:32] = s_right * q_group[:, 16:32].astype(np.float32)

    return fp32_to_bf16_rne(out.reshape(-1))


# ============================================================================
# 3. ZERO-COPY GGUF READER (HEADERS, METADATA & TENSOR MAP)
# ============================================================================

class GGUFReader:
    """Zero-copy memory-mapped GGUF v2/v3 parser."""

    def __init__(self, path: Path):
        self.path = path
        self.file = open(path, "rb")
        self.mm = mmap.mmap(self.file.fileno(), 0, access=mmap.ACCESS_READ)
        self.pos = 0

        self.version = 0
        self.tensor_count = 0
        self.kv_count = 0
        self.metadata: Dict[str, Any] = {}
        self.tensors: Dict[str, GGUFTensorInfo] = {}
        self.data_offset = 0

        self._parse_header_and_index()

    def close(self) -> None:
        self.mm.close()
        self.file.close()

    def __enter__(self) -> "GGUFReader":
        return self

    def __exit__(self, exc_type: Any, exc_val: Any, exc_tb: Any) -> None:
        self.close()

    def _read_struct(self, st: struct.Struct) -> Tuple[Any, ...]:
        val = st.unpack_from(self.mm, self.pos)
        self.pos += st.size
        return val

    def _read_string(self) -> str:
        (length,) = self._read_struct(SCALAR_STRUCTS[GGUF_TYPE_UINT64])
        raw = self.mm[self.pos : self.pos + length]
        self.pos += length
        return raw.decode("utf-8", errors="replace")

    def _read_value(self, vtype: int) -> Any:
        if vtype == GGUF_TYPE_STRING:
            return self._read_string()
        if vtype == GGUF_TYPE_ARRAY:
            (elem_type,) = self._read_struct(SCALAR_STRUCTS[GGUF_TYPE_UINT32])
            (count,) = self._read_struct(SCALAR_STRUCTS[GGUF_TYPE_UINT64])
            if elem_type == GGUF_TYPE_STRING:
                return [self._read_string() for _ in range(count)]
            st = SCALAR_STRUCTS[elem_type]
            total_bytes = st.size * count
            raw = self.mm[self.pos : self.pos + total_bytes]
            self.pos += total_bytes
            return np.frombuffer(raw, dtype=st.format).tolist()
        st = SCALAR_STRUCTS.get(vtype)
        if st is None:
            raise ValueError(f"Unknown GGUF metadata value type: {vtype}")
        return self._read_struct(st)[0]

    def _parse_header_and_index(self) -> None:
        (magic, version, tensor_count, kv_count) = struct.unpack_from("<IIQQ", self.mm, 0)
        self.pos = 24
        if magic != GGUF_MAGIC:
            raise ValueError(f"{self.path} is not a valid GGUF file (magic=0x{magic:08X})")
        if version not in (2, 3):
            raise ValueError(f"Unsupported GGUF version: {version}")

        self.version = version
        self.tensor_count = tensor_count
        self.kv_count = kv_count

        for _ in range(kv_count):
            key = self._read_string()
            (vtype,) = self._read_struct(SCALAR_STRUCTS[GGUF_TYPE_UINT32])
            self.metadata[key] = self._read_value(vtype)

        for _ in range(tensor_count):
            name = self._read_string()
            (n_dims,) = self._read_struct(SCALAR_STRUCTS[GGUF_TYPE_UINT32])
            dims = tuple(
                self._read_struct(SCALAR_STRUCTS[GGUF_TYPE_UINT64])[0]
                for _ in range(n_dims)
            )
            (ggml_type,) = self._read_struct(SCALAR_STRUCTS[GGUF_TYPE_UINT32])
            (offset,) = self._read_struct(SCALAR_STRUCTS[GGUF_TYPE_UINT64])
            self.tensors[name] = GGUFTensorInfo(
                name=name, dims=dims, ggml_type=ggml_type, offset=offset
            )

        alignment = int(self.metadata.get("general.alignment", GGUF_DEFAULT_ALIGNMENT))
        self.data_offset = (self.pos + alignment - 1) // alignment * alignment

    def get_tensor_bytes(self, name: str) -> Tuple[memoryview, GGUFTensorInfo]:
        info = self.tensors[name]
        start = self.data_offset + info.offset
        end = start + info.byte_size
        return memoryview(self.mm)[start:end], info


# ============================================================================
# 4. LOCAL CACHE & OLLAMA MANIFEST AUDITOR
# ============================================================================

def scan_local_models() -> List[Dict[str, Any]]:
    """
    Executes the local audit across Hugging Face cache and Ollama model manifests,
    inspecting GGUF magic headers and architecture metadata.
    """
    discovered: List[Dict[str, Any]] = []
    home = Path.home()

    # 1. Check Ollama Model Manifests
    ollama_roots = [
        Path(os.environ.get("OLLAMA_MODELS", "")) if os.environ.get("OLLAMA_MODELS") else None,
        home / ".ollama" / "models",
    ]
    for root in filter(None, ollama_roots):
        manifest_dir = root / "manifests"
        blobs_dir = root / "blobs"
        if not manifest_dir.exists() or not blobs_dir.exists():
            continue
        for mf_path in manifest_dir.rglob("*"):
            if not mf_path.is_file():
                continue
            try:
                data = json.loads(mf_path.read_text(encoding="utf-8"))
                for layer in data.get("layers", []):
                    if "model" in layer.get("mediaType", ""):
                        digest = layer.get("digest", "").replace(":", "-")
                        blob_path = blobs_dir / digest
                        if blob_path.exists():
                            rel_tag = mf_path.relative_to(manifest_dir).as_posix()
                            discovered.append({"tag": rel_tag, "path": blob_path, "source": "Ollama"})
            except Exception:
                continue

    # 2. Check Local Hugging Face Cache & Working Directory
    hf_roots = [
        Path(os.environ.get("HF_HOME", "")) if os.environ.get("HF_HOME") else None,
        home / ".cache" / "huggingface" / "hub",
        Path("C:/Models"),
        Path.cwd(),
    ]
    for root in filter(None, hf_roots):
        if not root.exists():
            continue
        for gguf_path in root.rglob("*.gguf"):
            if gguf_path.is_file():
                discovered.append({"tag": gguf_path.name, "path": gguf_path, "source": "HF/Local"})

    # Inspect GGUF magic & architecture strings
    results: List[Dict[str, Any]] = []
    seen_paths = set()
    for item in discovered:
        path = item["path"].resolve()
        if path in seen_paths:
            continue
        seen_paths.add(path)
        try:
            with open(path, "rb") as f:
                magic = struct.unpack("<I", f.read(4))[0]
            if magic != GGUF_MAGIC:
                continue
            with GGUFReader(path) as reader:
                arch = str(reader.metadata.get("general.architecture", "unknown"))
                blocks = reader.metadata.get(f"{arch}.block_count", 0)
                embd = reader.metadata.get(f"{arch}.embedding_length", 0)
                size_gb = path.stat().st_size / (1024 ** 3)
                results.append({
                    "tag": item["tag"],
                    "source": item["source"],
                    "path": path,
                    "arch": arch,
                    "layers": blocks,
                    "hidden_dim": embd,
                    "size_gb": round(size_gb, 2),
                    "axiom_compatible": arch in ("qwen2", "qwen25vl", "qwen3"),
                })
        except Exception:
            continue

    return results


# ============================================================================
# 5. AXIOM CONVERTER ENGINE (.axiom & _vocab.axiom)
# ============================================================================

def _build_gpt2_byte_decoder() -> Dict[str, int]:
    """
    Reverses GPT-2 / Qwen Byte-Level BPE unicode mapping back to raw bytes (0..255)
    so `_vocab.axiom` contains exact UTF-8 byte sequences for `app.exe`.
    """
    bs = (
        list(range(ord("!"), ord("~") + 1))
        + list(range(ord("¡"), ord("¬") + 1))
        + list(range(ord("®"), ord("ÿ") + 1))
    )
    cs = bs[:]
    n = 0
    for b in range(256):
        if b not in bs:
            bs.append(b)
            cs.append(256 + n)
            n += 1
    return {chr(c): b for b, c in zip(bs, cs)}


GPT2_BYTE_DECODER = _build_gpt2_byte_decoder()


def decode_bpe_token_to_bytes(token_str: str, token_type: int = 1) -> bytes:
    """
    Converts a GGUF token string into raw UTF-8 bytes.
    Control/special tokens (token_type == 3) are encoded directly as UTF-8;
    normal BPE tokens (token_type == 1) are decoded through the GPT-2 byte table.
    """
    if token_type == 3:
        return token_str.encode("utf-8")
    try:
        return bytes(GPT2_BYTE_DECODER[ch] for ch in token_str)
    except KeyError:
        return token_str.encode("utf-8")


def extract_axiom_config(reader: GGUFReader) -> Tuple[AxiomModelConfig, str]:
    arch = str(reader.metadata.get("general.architecture", "qwen2"))
    meta = reader.metadata

    def get_req(suffix: str, default: Optional[Any] = None) -> Any:
        key = f"{arch}.{suffix}"
        if key in meta:
            return meta[key]
        # Fallback across qwen2 / qwen25vl / qwen3 namespaces
        for alt in ("qwen2", "qwen25vl", "qwen3"):
            if f"{alt}.{suffix}" in meta:
                return meta[f"{alt}.{suffix}"]
        if default is not None:
            return default
        raise KeyError(f"Missing required GGUF metadata key: {key}")

    num_layers = int(get_req("block_count"))
    hidden_dim = int(get_req("embedding_length"))
    num_q_heads = int(get_req("attention.head_count"))
    num_kv_heads = int(get_req("attention.head_count_kv", num_q_heads))
    head_dim = int(get_req("attention.key_length", hidden_dim // num_q_heads))
    feedforward_network_dimension = int(get_req("feed_forward_length"))
    max_seq_len = int(get_req("context_length", 8192))
    rms_norm_eps = float(get_req("attention.layer_norm_rms_epsilon", 1e-6))
    rope_theta = float(get_req("rope.freq_base", 1000000.0))

    # Determine vocab_size from token_embd.weight shape or tokenizer list
    if "token_embd.weight" in reader.tensors:
        vocab_size = int(reader.tensors["token_embd.weight"].dims[1])
    else:
        tokens = meta.get("tokenizer.ggml.tokens", [])
        vocab_size = len(tokens)

    cfg = AxiomModelConfig(
        num_layers=num_layers,
        hidden_dim=hidden_dim,
        num_q_heads=num_q_heads,
        num_kv_heads=num_kv_heads,
        head_dim=head_dim,
        feedforward_network_dimension=feedforward_network_dimension,
        vocab_size=vocab_size,
        max_seq_len=max_seq_len,
        rms_norm_eps=rms_norm_eps,
        rope_theta=rope_theta,
    )
    return cfg, arch


def _write_tensor_bf16(
    out_f: BinaryIO,
    reader: GGUFReader,
    tensor_name: str,
    expected_elems: int,
    allow_zero_fallback: bool = False,
) -> str:
    """
    Streams a single GGUF tensor into `out_f` as contiguous BF16 values.
    If `allow_zero_fallback` is True and the tensor is absent (e.g., optional bias in Qwen3),
    writes zero-filled BF16 elements to preserve Axiom's fixed binary offset map.
    """
    if tensor_name not in reader.tensors:
        if allow_zero_fallback:
            zeros = np.zeros(expected_elems, dtype=np.uint16)
            out_f.write(zeros.tobytes())
            return "ZERO_BIAS_SYNTHESIZED"
        raise KeyError(f"Required tensor '{tensor_name}' not found in GGUF!")

    raw_mem, info = reader.get_tensor_bytes(tensor_name)
    if info.num_elements != expected_elems:
        raise ValueError(
            f"Shape mismatch on {tensor_name}: expected {expected_elems} elements "
            f"from AxiomModelConfig, found {info.num_elements} (dims={info.dims})"
        )

    bf16_arr = dequantize_to_bf16(raw_mem, info.ggml_type, info.num_elements)
    out_f.write(bf16_arr.tobytes())
    return info.type_name


def export_vocab_axiom(reader: GGUFReader, vocab_out_path: Path, decode_bpe: bool = True) -> int:
    """
    Writes `qwen_vocab.axiom`:
      - Bytes 0..3: uint32_t total_token_count
      - Repeated for each token:
          uint32_t token_id
          uint32_t text_length_bytes
          uint8_t[text_length_bytes] raw UTF-8 token string
    """
    tokens: List[str] = reader.metadata.get("tokenizer.ggml.tokens", [])
    token_types: List[int] = reader.metadata.get("tokenizer.ggml.token_type", [])
    if not tokens:
        raise ValueError("No `tokenizer.ggml.tokens` found in GGUF metadata.")

    vocab_out_path.parent.mkdir(parents=True, exist_ok=True)
    with open(vocab_out_path, "wb") as vf:
        vf.write(struct.pack("<I", len(tokens)))
        for idx, tok in enumerate(tokens):
            ttype = token_types[idx] if idx < len(token_types) else 1
            raw_bytes = decode_bpe_token_to_bytes(tok, ttype) if decode_bpe else tok.encode("utf-8")
            vf.write(struct.pack("<II", idx, len(raw_bytes)))
            vf.write(raw_bytes)

    return len(tokens)


def convert_gguf_to_axiom(
    gguf_path: Path,
    axiom_out_path: Path,
    vocab_out_path: Optional[Path] = None,
    decode_bpe: bool = True,
) -> None:
    with GGUFReader(gguf_path) as reader:
        cfg, arch = extract_axiom_config(reader)

        # Filter out any vision tower tensors (e.g., `v.blk.*` in qwen2.5-vl) automatically
        vision_tensors = [k for k in reader.tensors if k.startswith("v.") or k.startswith("mm.")]

        print("=" * 72)
        print(f"PROJECT AXIOM — GGUF TO .AXIOM PRECISION CONVERTER")
        print("=" * 72)
        print(f"Source GGUF       : {gguf_path}")
        print(f"Detected Arch     : {arch} ({len(vision_tensors)} vision tensors ignored)")
        print(f"Layers            : {cfg.num_layers}")
        print(f"Hidden Dim        : {cfg.hidden_dim}")
        print(f"Attention Heads   : {cfg.num_q_heads} Q / {cfg.num_kv_heads} KV (head_dim={cfg.head_dim})")
        print(f"SwiGLU FFN Dim    : {cfg.feedforward_network_dimension}")
        print(f"Vocab Size        : {cfg.vocab_size}")
        print(f"Max Seq Len       : {cfg.max_seq_len}")
        print(f"RMSNorm Eps       : {cfg.rms_norm_eps:g}")
        print(f"RoPE Theta        : {cfg.rope_theta:g}")
        print("-" * 72)

        q_dim = cfg.num_q_heads * cfg.head_dim
        kv_dim = cfg.num_kv_heads * cfg.head_dim

        axiom_out_path.parent.mkdir(parents=True, exist_ok=True)
        with open(axiom_out_path, "wb") as out_f:
            # 1. Write 64-byte AxiomModelConfig header
            out_f.write(cfg.pack())

            # 2. Write Token Embeddings: model.embed_tokens.weight
            t_type = _write_tensor_bf16(
                out_f, reader, "token_embd.weight", cfg.vocab_size * cfg.hidden_dim
            )
            print(f"[Embed] token_embd.weight -> model.embed_tokens.weight ({t_type} -> BF16 RNE)")

            # 3. Stream Transformer Layers sequentially (0 .. num_layers - 1)
            for l in range(cfg.num_layers):
                prefix = f"blk.{l}"
                layer_specs = [
                    # (GGUF name, expected element count, allow_zero_fallback)
                    (f"{prefix}.attn_norm.weight", cfg.hidden_dim, False),
                    (f"{prefix}.attn_q.weight", cfg.hidden_dim * q_dim, False),
                    (f"{prefix}.attn_q.bias", q_dim, True),
                    (f"{prefix}.attn_k.weight", cfg.hidden_dim * kv_dim, False),
                    (f"{prefix}.attn_k.bias", kv_dim, True),
                    (f"{prefix}.attn_v.weight", cfg.hidden_dim * kv_dim, False),
                    (f"{prefix}.attn_v.bias", kv_dim, True),
                    (f"{prefix}.attn_output.weight", q_dim * cfg.hidden_dim, False),
                    (f"{prefix}.ffn_norm.weight", cfg.hidden_dim, False),
                    (f"{prefix}.ffn_gate.weight", cfg.hidden_dim * cfg.feedforward_network_dimension, False),
                    (f"{prefix}.ffn_up.weight", cfg.hidden_dim * cfg.feedforward_network_dimension, False),
                    (f"{prefix}.ffn_down.weight", cfg.feedforward_network_dimension * cfg.hidden_dim, False),
                ]

                types_used = set()
                for t_name, expected_elems, optional_bias in layer_specs:
                    used = _write_tensor_bf16(out_f, reader, t_name, expected_elems, optional_bias)
                    types_used.add(used)

                print(
                    f"[Layer {l:02d}/{cfg.num_layers - 1:02d}] "
                    f"Streamed QKV+Bias, O-Proj, Norms, SwiGLU MLP ({', '.join(sorted(types_used))} -> BF16 RNE)"
                )

            # 4. Write Final RMSNorm: model.norm.weight
            norm_type = _write_tensor_bf16(
                out_f, reader, "output_norm.weight", cfg.hidden_dim
            )
            print(f"[Norm ] output_norm.weight -> model.norm.weight ({norm_type} -> BF16 RNE)")

            # 5. Write LM Head Projection: lm_head.weight (fallback to token_embd.weight if tied)
            lm_head_src = "output.weight" if "output.weight" in reader.tensors else "token_embd.weight"
            head_type = _write_tensor_bf16(
                out_f, reader, lm_head_src, cfg.vocab_size * cfg.hidden_dim
            )
            print(f"[Head ] {lm_head_src} -> lm_head.weight ({head_type} -> BF16 RNE)")

        total_bytes = axiom_out_path.stat().st_size
        print("-" * 72)
        print(f"Saved .axiom checkpoint : {axiom_out_path} ({total_bytes / (1024**3):.2f} GB)")

        if vocab_out_path is not None:
            tok_count = export_vocab_axiom(reader, vocab_out_path, decode_bpe=decode_bpe)
            v_bytes = vocab_out_path.stat().st_size
            print(f"Saved _vocab.axiom      : {vocab_out_path} ({tok_count} tokens, {v_bytes / (1024**2):.2f} MB)")
        print("=" * 72)


# ============================================================================
# 6. CLI ENTRY POINT
# ============================================================================

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Project Axiom — High-Precision GGUF to .axiom Converter & Local Auditor"
    )
    parser.add_argument(
        "--scan",
        action="store_true",
        help="Scan local Hugging Face cache & Ollama manifests and rank Axiom compatibility.",
    )
    parser.add_argument(
        "--input", "-i",
        type=str,
        help="Path to local .gguf file, Ollama blob sha256, or model substring (e.g. 'qwen2.5-vl').",
    )
    parser.add_argument(
        "--output", "-o",
        type=Path,
        default=Path("models/qwen7b.axiom"),
        help="Output .axiom binary checkpoint path (default: models/qwen7b.axiom).",
    )
    parser.add_argument(
        "--vocab", "-v",
        type=Path,
        default=Path("models/qwen_vocab.axiom"),
        help="Output _vocab.axiom path (default: models/qwen_vocab.axiom).",
    )
    parser.add_argument(
        "--raw-bpe-strings",
        action="store_true",
        help="Write raw GGUF BPE strings instead of decoding GPT-2 byte-level UTF-8.",
    )

    args = parser.parse_args()

    if args.scan or not args.input:
        models = scan_local_models()
        print(f"Discovered {len(models)} local GGUF model(s):")
        for m in models:
            status = "100% AXIOM MATCH" if m["axiom_compatible"] else "INCOMPATIBLE KERNELS"
            print(
                f"  - [{m['source']}] {m['tag']} | arch={m['arch']} | "
                f"layers={m['layers']} | hidden={m['hidden_dim']} | "
                f"{m['size_gb']} GB | {status}\n    Path: {m['path']}"
            )
        if not args.input:
            return

    input_path = Path(args.input)
    if not input_path.exists():
        # Attempt to auto-resolve against local Ollama / HF scan by tag or sha256 prefix
        matches = [
            m["path"] for m in scan_local_models()
            if args.input.lower() in m["tag"].lower() or args.input.lower() in m["path"].name.lower()
        ]
        if not matches:
            sys.exit(f"Error: Could not find GGUF file or matching local manifest for '{args.input}'.")
        input_path = matches[0]
        print(f"Resolved '{args.input}' -> {input_path}")

    convert_gguf_to_axiom(
        gguf_path=input_path,
        axiom_out_path=args.output,
        vocab_out_path=args.vocab,
        decode_bpe=not args.raw_bpe_strings,
    )


if __name__ == "__main__":
    main()

