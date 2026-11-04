#!/usr/bin/env python3
"""
Project Axiom — High-Density Systems & CUDA Corpus Generator
File: core/asset_sample/generate_systems_corpus.py

Generates a comprehensive, ChatML-formatted (<|im_start|>...<|im_end|>)
training corpus for Project Axiom. Covers:
1. High-Performance CUDA Kernel Optimization (warp intrinsics, uint4 coalescing, shared memory tiling)
2. Modern C++23 Systems Idioms (concepts, std::expected, std::span, lock-free ring buffers)
3. Zero-Copy Memory Management (Win32 DMA memory mapping, page alignment, caching allocators)
4. Mathematical & Algorithmic Proofs (Softmax Jacobian, RMSNorm gradients, RoPE rotation)
5. Real-World Profiling & Debugging Scenarios (eliminating bank conflicts, stream sync)
"""

from __future__ import annotations

import argparse
import os
import shutil
from pathlib import Path
from typing import List, Dict, Any


def build_cuda_kernel_corpus() -> List[Dict[str, str]]:
    """Category A: Advanced CUDA Kernel Optimization & GPU Architecture."""
    examples = []

    # 1. 128-bit Vectorized Loads
    examples.append({
        "prompt": "How do you optimize memory bandwidth saturation in a CUDA GEMV inner loop using 128-bit vectorized loads?",
        "response": """To maximize GDDR6/HBM memory bus utilization, warp threads should issue 128-bit memory instructions (`uint4` or `float4`) rather than scalar 16-bit or 32-bit loads. A single `uint4` instruction loads 16 bytes per thread (512 bytes per 32-thread warp), fully saturating 128-byte cache lines in coalesced transactions.

### Implementation Pattern (BF16 GEMV with `uint4`):
```cuda
#include <cuda_bf16.h>
#include <cuda_runtime.h>

__global__ void gemv_vectorized_uint4_bf16(
    int M, int N, int K,
    const __nv_bfloat16* __restrict__ X,
    const __nv_bfloat16* __restrict__ W,
    __nv_bfloat16* __restrict__ Y
) {
    int warp_id = threadIdx.x >> 5;
    int lane    = threadIdx.x & 31;
    int col     = blockIdx.x * 4 + warp_id;
    if (col >= N) return;

    const __nv_bfloat16* w_row = W + static_cast<size_t>(col) * K;
    float accum = 0.0f;

    // Process 8 __nv_bfloat16 (16 bytes = 128 bits) per thread iteration
    int k_vec_end = (K >> 8) << 8;
    for (int base_k = 0; base_k < k_vec_end; base_k += 256) {
        int cur_k = base_k + (lane << 3);
        uint4 x4 = *reinterpret_cast<const uint4*>(X + cur_k);
        uint4 w4 = *reinterpret_cast<const uint4*>(w_row + cur_k);

        const __nv_bfloat162* x_pairs = reinterpret_cast<const __nv_bfloat162*>(&x4);
        const __nv_bfloat162* w_pairs = reinterpret_cast<const __nv_bfloat162*>(&w4);

        #pragma unroll
        for (int p = 0; p < 4; ++p) {
            float2 fx = __bfloat1622float2(x_pairs[p]);
            float2 fw = __bfloat1622float2(w_pairs[p]);
            accum += fx.x * fw.x + fx.y * fw.y;
        }
    }

    // Residual scalar loop for remaining elements
    for (int cur_k = k_vec_end + lane; cur_k < K; cur_k += 32) {
        accum += __bfloat162float(X[cur_k]) * __bfloat162float(w_row[cur_k]);
    }

    // Warp-level butterfly reduction
    #pragma unroll
    for (int offset = 16; offset > 0; offset >>= 1) {
        accum += __shfl_down_sync(0xffffffff, accum, offset);
    }

    if (lane == 0) {
        Y[col] = __float2bfloat16(accum);
    }
}
```

### Architectural Rationale:
1. **Instruction Issue Rate:** Issuing 1 `LDG.E.128` replaces 8 separate `LDG.E.16` scalar instructions, reducing instruction queue pressure by 8x.
2. **Coalescing Efficiency:** 32 lanes loading 16 bytes each fetch 512 bytes sequentially, creating exactly 4 burst transactions on the 128-byte L2 cache boundary."""
    })

    # 2. Bank Conflict Elimination in 2D Shared Memory Transpose
    examples.append({
        "prompt": "Explain shared memory bank conflicts in CUDA and provide an implementation of a conflict-free matrix transpose.",
        "response": """In NVIDIA architectures, shared memory is divided into 32 banks (4 bytes wide each, matching warp size). When multiple threads within the same warp access distinct memory locations within the same bank simultaneously, accesses are serialized, severely degrading bandwidth.

### The Conflict Problem:
In a 32x32 float matrix (`tile[32][32]`), reading row `i` is conflict-free because lane `k` accesses address `i*32 + k`, mapping to bank `k`. However, reading column `k` causes lane `i` to access address `i*32 + k`, meaning all 32 lanes access bank `(i*32 + k) % 32 = k % 32`—a **32-way bank conflict**!

### The Solution: Stride Padding
By adding padding (`tile[32][33]`), row length becomes 33. Column access address becomes `i*33 + k`. Lane `i` now accesses bank `(i*33 + k) % 32 = (i + k) % 32`, giving an exact 1-to-1 bijection to all 32 banks with **zero serialization**.

```cuda
#include <cuda_runtime.h>

constexpr int TILE_DIM = 32;
constexpr int BLOCK_ROWS = 8;

__global__ void conflict_free_transpose(
    const float* __restrict__ idata,
    float* __restrict__ odata,
    int width, int height
) {
    // 33 columns eliminates 32-way bank conflicts during column readout
    __shared__ float tile[TILE_DIM][TILE_DIM + 1];

    int x = blockIdx.x * TILE_DIM + threadIdx.x;
    int y = blockIdx.y * TILE_DIM + threadIdx.y;

    #pragma unroll
    for (int j = 0; j < TILE_DIM; j += BLOCK_ROWS) {
        if (x < width && (y + j) < height) {
            tile[threadIdx.y + j][threadIdx.x] = idata[(y + j) * width + x];
        }
    }
    __syncthreads();

    x = blockIdx.y * TILE_DIM + threadIdx.x;
    y = blockIdx.x * TILE_DIM + threadIdx.y;

    #pragma unroll
    for (int j = 0; j < TILE_DIM; j += BLOCK_ROWS) {
        if (x < height && (y + j) < width) {
            odata[(y + j) * height + x] = tile[threadIdx.x][threadIdx.y + j];
        }
    }
}
```"""
    })

    # 3. Fused Softmax & Argmax via Warp Shuffles
    examples.append({
        "prompt": "Write a CUDA kernel for fused single-pass Softmax and Argmax that eliminates host PCIe transfer overhead during greedy LLM decoding.",
        "response": """In standard LLM autoregressive decoding, copying the full logits tensor (e.g. 152,064 floats = 608 KB) to CPU on every token stalls GPU execution and consumes host PCIe bandwidth. A fused GPU kernel can compute max reduction, Softmax scaling, and Argmax within GPU warp registers, transferring only a single 4-byte `uint32_t` predicted token ID to the host.

```cuda
#include <cuda_runtime.h>
#include <cstdint>
#include <cmath>

__global__ void softmax_argmax_fused_kernel(
    float* __restrict__ logits_and_probs,
    uint32_t* __restrict__ pred_id,
    size_t vocab_size
) {
    __shared__ float s_max[32];
    __shared__ uint32_t s_arg[32];
    __shared__ float s_sum[32];

    int lane = threadIdx.x & 31;
    int wid  = threadIdx.x >> 5;

    float local_max = -1e30f;
    uint32_t local_arg = 0;

    // 1. Thread-local max & argmax search
    for (size_t i = threadIdx.x; i < vocab_size; i += blockDim.x) {
        float val = logits_and_probs[i];
        if (isfinite(val) && val > local_max) {
            local_max = val;
            local_arg = static_cast<uint32_t>(i);
        }
    }

    // Warp-level reduction
    #pragma unroll
    for (int offset = 16; offset > 0; offset >>= 1) {
        float other_max   = __shfl_down_sync(0xffffffff, local_max, offset);
        uint32_t other_arg = __shfl_down_sync(0xffffffff, local_arg, offset);
        if (other_max > local_max) {
            local_max = other_max;
            local_arg = other_arg;
        }
    }

    if (lane == 0) {
        s_max[wid] = local_max;
        s_arg[wid] = local_arg;
    }
    __syncthreads();

    // Block-level max consolidation by warp 0
    if (wid == 0) {
        float warp_max   = (lane < (blockDim.x >> 5)) ? s_max[lane] : -1e30f;
        uint32_t warp_arg = (lane < (blockDim.x >> 5)) ? s_arg[lane] : 0;
        #pragma unroll
        for (int offset = 16; offset > 0; offset >>= 1) {
            float other_max   = __shfl_down_sync(0xffffffff, warp_max, offset);
            uint32_t other_arg = __shfl_down_sync(0xffffffff, warp_arg, offset);
            if (other_max > warp_max) {
                warp_max = other_max;
                warp_arg = other_arg;
            }
        }
        if (lane == 0) {
            s_max[0] = warp_max;
            if (pred_id) *pred_id = warp_arg;
        }
    }
    __syncthreads();

    // 2. Numerically stable exponentiation & normalization
    float global_max = s_max[0];
    float sum_exp = 0.0f;
    for (size_t i = threadIdx.x; i < vocab_size; i += blockDim.x) {
        float exp_val = expf(fmaxf(-50.0f, fminf(50.0f, logits_and_probs[i] - global_max)));
        logits_and_probs[i] = exp_val;
        sum_exp += exp_val;
    }

    #pragma unroll
    for (int offset = 16; offset > 0; offset >>= 1) {
        sum_exp += __shfl_down_sync(0xffffffff, sum_exp, offset);
    }
    if (lane == 0) s_sum[wid] = sum_exp;
    __syncthreads();

    if (wid == 0) {
        float warp_sum = (lane < (blockDim.x >> 5)) ? s_sum[lane] : 0.0f;
        #pragma unroll
        for (int offset = 16; offset > 0; offset >>= 1) {
            warp_sum += __shfl_down_sync(0xffffffff, warp_sum, offset);
        }
        if (lane == 0) s_sum[0] = warp_sum > 1e-12f ? warp_sum : 1.0f;
    }
    __syncthreads();

    float inv_sum = 1.0f / s_sum[0];
    for (size_t i = threadIdx.x; i < vocab_size; i += blockDim.x) {
        logits_and_probs[i] *= inv_sum;
    }
}
```"""
    })

    # 4. FlashAttention Online Softmax
    examples.append({
        "prompt": "Explain the FlashAttention online Softmax formulation and how it avoids writing N x N attention matrices to HBM.",
        "response": r"""Standard attention computes $S = Q K^T / \sqrt{d}$, materializes $S \in \mathbb{R}^{N \times N}$ in high-bandwidth memory (HBM), computes $P = \text{softmax}(S)$, and multiplies $O = P V$. For large contexts ($N \ge 4096$), $O(N^2)$ HBM memory traffic dominates execution latency.

FlashAttention tiles $Q, K, V$ into SRAM blocks ($B_r, B_c$). To compute Softmax incrementally across blocks without materializing the full score row, it uses **Milakov & Gimelshein's Online Normalizer**:

### Online Softmax Equations:
For incoming tile score vector $s^{(j)}$:
1. Local tile max: $m^{(j)} = \max(s^{(j)})$
2. Updated global max: $m^{\text{new}} = \max(m^{\text{old}}, m^{(j)})$
3. Scaling factor for previous accumulator: $\alpha = e^{m^{\text{old}} - m^{\text{new}}}$
4. Current block exponent sum: $\ell^{(j)} = \sum e^{s^{(j)} - m^{\text{new}}}$
5. Updated global denominator: $\ell^{\text{new}} = \ell^{\text{old}} \cdot \alpha + \ell^{(j)}$
6. Rescaled output vector:
   $$O^{\text{new}} = O^{\text{old}} \cdot \alpha + \sum e^{s^{(j)} - m^{\text{new}}} V^{(j)}$$

### CUDA Tile Loop Structure:
```cuda
float m_prev = -1e30f;
float l_prev = 0.0f;
float acc[HEAD_DIM] = {0.0f};

for (int tile_idx = 0; tile_idx < num_tiles; ++tile_idx) {
    // 1. Compute local tile scores: s = q * k_tile^T / sqrt(d)
    float m_curr = local_max(s_tile);
    float m_new = fmaxf(m_prev, m_curr);
    float alpha = expf(m_prev - m_new);
    float beta  = expf(m_curr - m_new);

    // 2. Rescale previous accumulator
    #pragma unroll
    for (int d = 0; d < HEAD_DIM; ++d) {
        acc[d] *= alpha;
    }

    // 3. Accumulate v_tile * exp(s - m_new)
    accumulate_v(acc, s_tile, v_tile, m_new);

    l_prev = l_prev * alpha + local_sum_exp(s_tile, m_new);
    m_prev = m_new;
}

// Final normalization
float inv_l = 1.0f / (l_prev + 1e-8f);
for (int d = 0; d < HEAD_DIM; ++d) {
    out[d] = acc[d] * inv_l;
}
```"""
    })

    return examples


def build_cpp23_systems_corpus() -> List[Dict[str, str]]:
    """Category B: Modern C++23 Systems Idioms, Zero-Cost Abstractions & Concurrency."""
    examples = []

    # 1. Zero-Copy Win32 / POSIX Memory Mapping
    examples.append({
        "prompt": "Write a cross-platform, RAII-compliant C++23 zero-copy memory-mapped file reader for high-throughput model weights streaming.",
        "response": """Zero-copy memory mapping (`MapViewOfFile` on Windows, `mmap` on POSIX) maps file pages directly into the process virtual address space. This avoids user-space buffering and enables the operating system's virtual memory subsystem to DMA weights directly into pinned buffers.

```cpp
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <system_error>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

class MemoryMappedFile {
public:
    MemoryMappedFile() = default;
    ~MemoryMappedFile() noexcept { close(); }

    MemoryMappedFile(const MemoryMappedFile&) = delete;
    MemoryMappedFile& operator=(const MemoryMappedFile&) = delete;

    MemoryMappedFile(MemoryMappedFile&& other) noexcept {
        swap(other);
    }
    MemoryMappedFile& operator=(MemoryMappedFile&& other) noexcept {
        if (this != &other) {
            close();
            swap(other);
        }
        return *this;
    }

    static std::optional<MemoryMappedFile> open(const std::filesystem::path& path) {
        MemoryMappedFile map;
        if (!map.init(path)) return std::nullopt;
        return map;
    }

    [[nodiscard]] std::span<const uint8_t> bytes() const noexcept {
        return { data_, size_ };
    }

    [[nodiscard]] size_t size() const noexcept { return size_; }
    [[nodiscard]] const void* data() const noexcept { return data_; }

    void close() noexcept {
        if (!data_) return;
#ifdef _WIN32
        UnmapViewOfFile(data_);
        if (map_handle_) CloseHandle(map_handle_);
        if (file_handle_ != INVALID_HANDLE_VALUE) CloseHandle(file_handle_);
        file_handle_ = INVALID_HANDLE_VALUE;
        map_handle_ = nullptr;
#else
        munmap(const_cast<void*>(data_), size_);
        if (fd_ != -1) ::close(fd_);
        fd_ = -1;
#endif
        data_ = nullptr;
        size_ = 0;
    }

private:
    const void* data_ = nullptr;
    size_t size_ = 0;

#ifdef _WIN32
    HANDLE file_handle_ = INVALID_HANDLE_VALUE;
    HANDLE map_handle_ = nullptr;
#else
    int fd_ = -1;
#endif

    bool init(const std::filesystem::path& path) {
#ifdef _WIN32
        file_handle_ = CreateFileW(
            path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr
        );
        if (file_handle_ == INVALID_HANDLE_VALUE) return false;

        LARGE_INTEGER li;
        if (!GetFileSizeEx(file_handle_, &li)) return false;
        size_ = static_cast<size_t>(li.QuadPart);
        if (size_ == 0) return true;

        map_handle_ = CreateFileMappingW(file_handle_, nullptr, PAGE_READONLY, 0, 0, nullptr);
        if (!map_handle_) return false;

        data_ = MapViewOfFile(map_handle_, FILE_MAP_READ, 0, 0, 0);
        return data_ != nullptr;
#else
        fd_ = ::open(path.c_str(), O_RDONLY);
        if (fd_ == -1) return false;

        struct stat st;
        if (fstat(fd_, &st) == -1) return false;
        size_ = static_cast<size_t>(st.st_size);
        if (size_ == 0) return true;

        data_ = mmap(nullptr, size_, PROT_READ, MAP_SHARED, fd_, 0);
        return data_ != MAP_FAILED;
#endif
    }

    void swap(MemoryMappedFile& other) noexcept {
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
#ifdef _WIN32
        std::swap(file_handle_, other.file_handle_);
        std::swap(map_handle_, other.map_handle_);
#else
        std::swap(fd_, other.fd_);
#endif
    }
};
```"""
    })

    # 2. Lock-Free SPSC Ring Buffer
    examples.append({
        "prompt": "Implement a cache-aligned, lock-free Single-Producer Single-Consumer (SPSC) ring buffer in C++23 with acquire-release memory semantics.",
        "response": """In low-latency systems pipelines (such as streaming tokens from a background inference worker to an HTTP socket server), mutex locks introduce kernel context switches. A lock-free SPSC queue uses cache-line aligned atomic cursors with acquire-release ordering to achieve zero-contention, lockless IPC.

```cpp
#include <atomic>
#include <cstddef>
#include <new>
#include <optional>
#include <vector>

#if defined(__cpp_lib_hardware_interference_size)
using std::hardware_destructive_interference_size;
#else
constexpr size_t hardware_destructive_interference_size = 64;
#endif

template <typename T, size_t Capacity>
class SpscRingBuffer {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");

public:
    SpscRingBuffer() : buffer_(Capacity) {}

    bool push(const T& item) noexcept(std::is_nothrow_copy_constructible_v<T>) {
        const size_t head = head_.load(std::memory_order_relaxed);
        const size_t tail = tail_cache_;

        if ((head - tail) >= Capacity) {
            tail_cache_ = tail_.load(std::memory_order_acquire);
            if ((head - tail_cache_) >= Capacity) {
                return false; // Queue full
            }
        }

        buffer_[head & BufferMask] = item;
        head_.store(head + 1, std::memory_order_release);
        return true;
    }

    std::optional<T> pop() noexcept(std::is_nothrow_move_constructible_v<T>) {
        const size_t tail = tail_.load(std::memory_order_relaxed);
        const size_t head = head_cache_;

        if (tail == head) {
            head_cache_ = head_.load(std::memory_order_acquire);
            if (tail == head_cache_) {
                return std::nullopt; // Queue empty
            }
        }

        T item = std::move(buffer_[tail & BufferMask]);
        tail_.store(tail + 1, std::memory_order_release);
        return item;
    }

private:
    static constexpr size_t BufferMask = Capacity - 1;

    std::vector<T> buffer_;

    // Align cursors to separate cache lines to avoid false sharing
    alignas(hardware_destructive_interference_size) std::atomic<size_t> head_{0};
    alignas(hardware_destructive_interference_size) size_t tail_cache_{0};

    alignas(hardware_destructive_interference_size) std::atomic<size_t> tail_{0};
    alignas(hardware_destructive_interference_size) size_t head_cache_{0};
};
```

### Architectural Guarantees:
1. **Zero False Sharing:** Producer writes to `head_` while consumer writes to `tail_`. Placing them on separate 64-byte cache lines prevents L1/L2 cache invalidation storms.
2. **Acquire-Release Ordering:** The consumer's `acquire` load synchronizes-with the producer's `release` store, guaranteeing data is fully written to `buffer_` before the updated index is visible."""
    })

    # 3. C++20/23 Concepts for Tensor Validation
    examples.append({
        "prompt": "How do you enforce compile-time tensor layout and precision constraints in C++23 using concepts?",
        "response": """C++20/23 concepts allow hardware libraries to reject misaligned memory layouts, unsupported numerical types, or non-coalesced dimensions at compile time, eliminating runtime type checking overhead entirely.

```cpp
#include <concepts>
#include <cstdint>
#include <stdfloat>
#include <type_traits>

template <typename T>
concept TensorScalar = std::same_as<T, float> ||
                       std::same_as<T, std::bfloat16_t> ||
                       std::same_as<T, uint16_t>;

template <typename T>
concept Aligned128Bit = requires {
    requires alignof(T) >= 16;
};

template <typename T>
concept ContiguousTensorView = requires(T t) {
    { t.data() } -> std::contiguous_iterator;
    { t.size() } -> std::convertible_to<std::size_t>;
    requires TensorScalar<typename T::value_type>;
};

template <ContiguousTensorView InputView, ContiguousTensorView OutputView>
requires std::same_as<typename InputView::value_type, typename OutputView::value_type>
void launch_tensor_norm(const InputView& input, OutputView& output, float eps) {
    // Guaranteed by compiler: input and output are contiguous, have matching scalar types,
    // and hold supported floating-point/bfloat16 values.
}
```"""
    })

    return examples


def build_math_proofs_corpus() -> List[Dict[str, str]]:
    """Category C: Neural Network Algorithmic Derivations & Mathematical Proofs."""
    examples = []

    # 1. RMSNorm Forward & Backward Mathematical Derivation
    examples.append({
        "prompt": "Provide the complete mathematical derivation and CUDA backward gradient equations for Root Mean Square Normalization (RMSNorm).",
        "response": r"""Root Mean Square Normalization (RMSNorm) modifies LayerNorm by removing the mean-centering step, hypothesizing that the scaling property of LayerNorm provides the majority of gradient regulation benefits.

### 1. Forward Equations:
Given input vector $x \in \mathbb{R}^D$ and learnable gain $\gamma \in \mathbb{R}^D$:

$$\text{RMS}(x) = \sqrt{\frac{1}{D} \sum_{i=1}^D x_i^2 + \epsilon}$$

$$\hat{x}_i = \frac{x_i}{\text{RMS}(x)}$$

$$y_i = \gamma_i \odot \hat{x}_i$$

### 2. Backward Gradient Derivation:
Let $g_i = \frac{\partial \mathcal{L}}{\partial y_i}$ be the incoming gradient from the next layer.

#### Gain Gradient ($\gamma$):
$$\frac{\partial \mathcal{L}}{\partial \gamma_i} = g_i \cdot \hat{x}_i$$

#### Input Gradient ($x_i$):
By applying the multivariate chain rule:
$$\frac{\partial \mathcal{L}}{\partial x_i} = \frac{g_i \gamma_i}{\text{RMS}(x)} + \sum_{k=1}^D \left( g_k \gamma_k x_k \right) \cdot \frac{\partial}{\partial x_i}\left( \frac{1}{\text{RMS}(x)} \right)$$

Computing the derivative of the inverse RMS term:
$$\frac{\partial}{\partial x_i} \left( \left(\frac{1}{D}\sum x_j^2 + \epsilon\right)^{-1/2} \right) = -\frac{1}{2}\left(\text{RMS}(x)^2\right)^{-3/2} \cdot \frac{2 x_i}{D} = -\frac{x_i}{D \cdot \text{RMS}(x)^3}$$

Substituting back into the gradient sum:
$$\frac{\partial \mathcal{L}}{\partial x_i} = \frac{1}{\text{RMS}(x)} \left[ g_i \gamma_i - \frac{x_i}{D \cdot \text{RMS}(x)^2} \sum_{k=1}^D (g_k \gamma_k x_k) \right]$$

Since $\hat{x}_k = \frac{x_k}{\text{RMS}(x)}$, this simplifies to the exact fused kernel form:
$$\frac{\partial \mathcal{L}}{\partial x_i} = \frac{1}{\text{RMS}(x)} \left[ g_i \gamma_i - \frac{\hat{x}_i}{D} \sum_{k=1}^D (g_k \gamma_k \hat{x}_k) \right]$$

### 3. Fused CUDA Backward Implementation:
```cuda
__global__ void rmsnorm_backward_kernel(
    const __nv_bfloat16* __restrict__ grad_out,
    const __nv_bfloat16* __restrict__ input,
    const __nv_bfloat16* __restrict__ gamma,
    __nv_bfloat16* __restrict__ grad_in,
    int D, float eps
) {
    int tid = threadIdx.x;
    float sum_sq = 0.0f;
    for (int i = tid; i < D; i += blockDim.x) {
        float val = __bfloat162float(input[i]);
        sum_sq += val * val;
    }
    // Warp + shared memory reduce sum_sq...
    float inv_rms = rsqrtf(sum_sq / D + eps);

    float dot_g_x = 0.0f;
    for (int i = tid; i < D; i += blockDim.x) {
        float x_hat = __bfloat162float(input[i]) * inv_rms;
        float g     = __bfloat162float(grad_out[i]) * __bfloat162float(gamma[i]);
        dot_g_x += g * x_hat;
    }
    // Warp + shared memory reduce dot_g_x...

    float mean_dot = dot_g_x / static_cast<float>(D);
    for (int i = tid; i < D; i += blockDim.x) {
        float x_hat = __bfloat162float(input[i]) * inv_rms;
        float g     = __bfloat162float(grad_out[i]) * __bfloat162float(gamma[i]);
        grad_in[i]  = __float2bfloat16(inv_rms * (g - x_hat * mean_dot));
    }
}
```"""
    })

    # 2. Rotary Position Embedding (RoPE) Mathematical Invariance
    examples.append({
        "prompt": "Derive the mathematical property of Rotary Position Embedding (RoPE) and prove why the inner product depends only on relative distance (m - n).",
        "response": r"""Rotary Position Embedding (RoPE) encodes absolute token position $m$ by rotating the 2D affine subspaces of Query and Key vectors using complex numbers, such that their inner product inherently depends solely on relative token distance $m - n$.

### 1. Complex Number Formulation:
Group elements of vector $q \in \mathbb{R}^d$ into pairs:
$$z_k = (q_{2k}, q_{2k+1}) \equiv q_{2k} + i q_{2k+1} \in \mathbb{C}$$

Define position rotation angle $\theta_k = \Theta^{-2k/d}$ (where $\Theta = 10^6$ in modern architectures like Qwen).
At token position $m$, rotate the vector by $e^{i m \theta_k}$:
$$R_m(z_k) = z_k \cdot e^{i m \theta_k} = (q_{2k} + i q_{2k+1})(\cos(m\theta_k) + i\sin(m\theta_k))$$

Separating real and imaginary parts:
$$\text{Re}(R_m(z_k)) = q_{2k} \cos(m\theta_k) - q_{2k+1} \sin(m\theta_k)$$
$$\text{Im}(R_m(z_k)) = q_{2k+1} \cos(m\theta_k) + q_{2k} \sin(m\theta_k)$$

### 2. Inner Product Relative Distance Proof:
For Query at position $m$ ($q_m$) and Key at position $n$ ($k_n$), the dot product of the 2D subspace is given by the real part of their Hermitian inner product:
$$\langle R_m(q), R_n(k) \rangle = \text{Re}\left( (q \cdot e^{i m \theta}) \cdot (k \cdot e^{i n \theta})^* \right)$$

Using complex conjugation properties:
$$(k \cdot e^{i n \theta})^* = k^* \cdot e^{-i n \theta}$$

Substituting:
$$\langle R_m(q), R_n(k) \rangle = \text{Re}\left( q \cdot k^* \cdot e^{i m \theta} \cdot e^{-i n \theta} \right) = \text{Re}\left( q \cdot k^* \cdot e^{i (m - n) \theta} \right)$$

$$\langle R_m(q), R_n(k) \rangle = \langle q, R_{m-n}(k) \rangle$$

### 3. Conclusion:
The attention score $q_m^T k_n$ contains zero absolute positional bias; it depends **strictly on the relative displacement $(m - n)$**. This allows models to generalize seamlessly across context windows without positional parameter drift."""
    })

    return examples


def build_debugging_profiling_corpus() -> List[Dict[str, str]]:
    """Category D: Real-world Systems Profiling, Deadlock Resolution & CUDA Debugging."""
    examples = []

    examples.append({
        "prompt": "How do you detect and fix uncoalesced memory access and CUDA warp divergence using NVIDIA Nsight Compute?",
        "response": """In CUDA execution, memory throughput and compute pipeline efficiency depend on coalescing and uniform branch paths.

### 1. Uncoalesced Global Memory Access:
* **The Symptom:** Nsight Compute reports high `Memory Throughput %` but low `DRAM Bandwidth Utilization %`, accompanied by high `l1tex__t_sectors_pipe_lsu_mem_global_op_ld.sum`.
* **Root Cause:** A warp of 32 threads requests 32 separate 32-byte memory sectors instead of 1 contiguous 128-byte cache line (e.g., accessing strided data `X[threadIdx.x * stride]`).
* **The Fix:** Transpose data layout from Array-of-Structures (AoS) to Structure-of-Arrays (SoA), or load contiguous chunks into shared memory first before accessing strided elements.

### 2. Warp Divergence:
* **The Symptom:** High `smsp__warp_cycles_per_issue_active` and low `smsp__thread_inst_executed_per_inst_issued`.
* **Root Cause:** Threads within the same warp evaluate different branches of an `if-else` condition:
  ```cuda
  if (threadIdx.x % 2 == 0) {
      path_a();
  } else {
      path_b();
  }
  ```
  The warp must execute `path_a` with odd threads masked out, then execute `path_b` with even threads masked out—doubling execution time.
* **The Fix:** Structure thread grid mappings so entire warps (32 threads) evaluate identical branch outcomes (`threadIdx.x / 32` or block-level branching)."""
    })

    return examples


def format_chatml(prompt: str, response: str) -> str:
    """Encodes a QA pair into Qwen ChatML standard conversational format."""
    return f"<|im_start|>user\n{prompt.strip()}<|im_end|>\n<|im_start|>assistant\n{response.strip()}<|im_end|>\n"


def generate_corpus(output_path: Path, repeat_multiplier: int = 1) -> int:
    """Assembles all categories, formats in ChatML, and writes to target file."""
    all_examples = []
    all_examples.extend(build_cuda_kernel_corpus())
    all_examples.extend(build_cpp23_systems_corpus())
    all_examples.extend(build_math_proofs_corpus())
    all_examples.extend(build_debugging_profiling_corpus())

    output_path.parent.mkdir(parents=True, exist_ok=True)

    total_chars = 0
    with open(output_path, "w", encoding="utf-8") as f:
        for _ in range(repeat_multiplier):
            for ex in all_examples:
                block = format_chatml(ex["prompt"], ex["response"])
                f.write(block)
                total_chars += len(block)

    return total_chars


def main():
    parser = argparse.ArgumentParser(description="Project Axiom Systems & CUDA Corpus Generator")
    parser.add_argument(
        "--output", "-o",
        type=Path,
        default=Path("core/asset_sample/systems_training_corpus.txt"),
        help="Path to output corpus text file."
    )
    parser.add_argument(
        "--multiplier", "-m",
        type=int,
        default=5,
        help="Repeat count multiplier to scale corpus size for continuous context window ingestion."
    )
    parser.add_argument(
        "--archive-old-asm",
        action="store_true",
        default=True,
        help="Archive existing assembly_training_corpus.txt into core/asset_sample/archive/"
    )
    args = parser.parse_args()

    # Archive old assembly corpus if requested
    old_asm_path = Path("core/asset_sample/assembly_training_corpus.txt")
    if args.archive_old_asm and old_asm_path.exists():
        archive_dir = Path("core/asset_sample/archive")
        archive_dir.mkdir(parents=True, exist_ok=True)
        dest = archive_dir / "assembly_training_corpus.txt"
        shutil.move(str(old_asm_path), str(dest))
        print(f"[Axiom Corpus] Archived legacy assembly corpus to: {dest}")

    print(f"[Axiom Corpus] Generating high-density ChatML systems corpus...")
    total_bytes = generate_corpus(args.output, args.multiplier)
    print(f"[Axiom Corpus] Successfully generated {args.output} ({total_bytes / 1024:.1f} KB, multiplier={args.multiplier}).")
    print(f"[Axiom Corpus] Ready for training via: .\\core\\app.exe --train or .\\core\\app.exe --bulk")


if __name__ == "__main__":
    main()
