import json
import struct
import sys
from pathlib import Path
import numpy as np

def export_qwen_to_axiom(model_dir: str, out_weights: str, out_vocab: str):
    model_dir_path = Path(model_dir)
    safetensors_path = model_dir_path / "model.safetensors"
    tokenizer_path = model_dir_path / "tokenizer.json"

    if not safetensors_path.exists():
        print(f"[Axiom Converter] Error: {safetensors_path} does not exist.")
        return False

    print(f"[Axiom Converter] Reading Safetensors header from {safetensors_path}...")
    with open(safetensors_path, "rb") as f_in:
        header_len = struct.unpack("<Q", f_in.read(8))[0]
        header_json = f_in.read(header_len).decode("utf-8")
        meta_table = json.loads(header_json)
        base_offset = 8 + header_len

        # Header: magic(0x4D4F4958="MOIX"), version(2), num_layers(28), hidden_dim(1536),
        # num_q_heads(12), num_kv_heads(2), head_dim(128), ffn_dim(8960), vocab_size(151936),
        # max_seq_len(4096), rms_norm_eps(1e-6), rope_theta(1000000.0) + 16 bytes reserved
        header = struct.pack(
            "<10I2f16x",
            0x4D4F4958, 2, 28, 1536, 12, 2, 128, 8960, 151936, 4096,
            1e-6, 1000000.0
        )

        def write_tensor_bf16(f_out, tensor_name: str):
            if tensor_name not in meta_table:
                raise KeyError(f"Missing required tensor: {tensor_name}")
            t_info = meta_table[tensor_name]
            start, end = t_info["data_offsets"]
            dtype = t_info["dtype"]
            f_in.seek(base_offset + start)
            raw = f_in.read(end - start)

            if dtype == "BF16":
                f_out.write(raw)
            elif dtype == "F32":
                u32 = np.frombuffer(raw, dtype=np.uint32)
                bf16 = (u32 >> 16).astype(np.uint16).tobytes()
                f_out.write(bf16)
            elif dtype == "F16":
                f16 = np.frombuffer(raw, dtype=np.float16)
                f32 = f16.astype(np.float32)
                u32 = np.frombuffer(f32.tobytes(), dtype=np.uint32)
                bf16 = (u32 >> 16).astype(np.uint16).tobytes()
                f_out.write(bf16)
            else:
                raise ValueError(f"Unsupported tensor dtype {dtype} for {tensor_name}")

        Path(out_weights).parent.mkdir(parents=True, exist_ok=True)
        Path(out_vocab).parent.mkdir(parents=True, exist_ok=True)

        print(f"[Axiom Converter] Streaming weights into binary checkpoint: {out_weights}...")
        with open(out_weights, "wb") as out:
            out.write(header)
            write_tensor_bf16(out, "model.embed_tokens.weight")

            for l in range(28):
                if (l + 1) % 4 == 0 or l == 27:
                    print(f"  [..] Packing transformer block [{l + 1}/28]...")
                p = f"model.layers.{l}"
                write_tensor_bf16(out, f"{p}.input_layernorm.weight")
                write_tensor_bf16(out, f"{p}.self_attn.q_proj.weight")
                write_tensor_bf16(out, f"{p}.self_attn.q_proj.bias")
                write_tensor_bf16(out, f"{p}.self_attn.k_proj.weight")
                write_tensor_bf16(out, f"{p}.self_attn.k_proj.bias")
                write_tensor_bf16(out, f"{p}.self_attn.v_proj.weight")
                write_tensor_bf16(out, f"{p}.self_attn.v_proj.bias")
                write_tensor_bf16(out, f"{p}.self_attn.o_proj.weight")
                write_tensor_bf16(out, f"{p}.post_attention_layernorm.weight")
                write_tensor_bf16(out, f"{p}.mlp.gate_proj.weight")
                write_tensor_bf16(out, f"{p}.mlp.up_proj.weight")
                write_tensor_bf16(out, f"{p}.mlp.down_proj.weight")

            write_tensor_bf16(out, "model.norm.weight")
            if "lm_head.weight" in meta_table:
                write_tensor_bf16(out, "lm_head.weight")
            else:
                write_tensor_bf16(out, "model.embed_tokens.weight")

    if tokenizer_path.exists():
        print(f"[Axiom Converter] Exporting BPE vocabulary from {tokenizer_path}...")
        tok_data = json.loads(tokenizer_path.read_text(encoding="utf-8"))
        vocab = tok_data.get("model", {}).get("vocab", {})
        for added in tok_data.get("added_tokens", []):
            vocab[added["content"]] = added["id"]

        with open(out_vocab, "wb") as vf:
            vf.write(struct.pack("<I", len(vocab)))
            for token_str, token_id in vocab.items():
                raw_str = token_str.replace("Ġ", " ").replace("Ċ", "\n").encode("utf-8")
                vf.write(struct.pack("<II", int(token_id), len(raw_str)))
                vf.write(raw_str)
        print(f"[Axiom Converter] Vocabulary saved: {out_vocab} ({len(vocab)} tokens)")

    print(f"[Axiom Converter] Successfully generated {out_weights} and {out_vocab}.")
    return True

if __name__ == "__main__":
    candidates = [
        sys.argv[1] if len(sys.argv) > 1 else None,
        "models/qwen_hf",
        "./Qwen2.5-Coder-1.5B-Instruct"
    ]
    src_dir = next((c for c in candidates if c and Path(c).exists()), None)
    if not src_dir:
        src_dir = candidates[1]
    
    out_w = sys.argv[2] if len(sys.argv) > 2 else "models/qwen1_5b.axiom"
    out_v = sys.argv[3] if len(sys.argv) > 3 else "models/qwen_vocab.axiom"

    export_qwen_to_axiom(src_dir, out_w, out_v)
