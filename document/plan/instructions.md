This unified, step-by-step execution blueprint reconciles your **Project Axiom v1 Core Roadmap** (fixing the Phase 2 synchronization liabilities, batching Tensor Cores at $M=32$, and stacking Transformer blocks) with the **Qwen2.5 + Zero-PyTorch ComfyUI Integration** into a single chronological build order.

## Master Execution Order

| Stage | Subsystem Target | Primary Files Modified / Created | Deliverable Outcome |
| --- | --- | --- | --- |
| **Stage 1** | Sync & VRAM Eradication | `upload.cu`, `download.cu`, `optimizer.cpp` | Zero runtime `cudaMalloc` or `cudaDeviceSynchronize` |
| **Stage 2** | Tokenizer & Batched GEMM | `parser.cpp`, `forward_pass.cu`, `backward_pass.cu` | $M=32$ Tensor Core saturation (4,000 tok/s) |
| **Stage 3** | Qwen / Transformer Kernels | `rmsnorm.cu`, `rope.cu`, `paged_gqa.cu`, `swiglu.cu` | Native `sm_89` BF16 operators for stacked layers |
| **Stage 4** | Multi-Layer Stacked Loop | `ai.hpp`, `ai.cpp` | $N$-layer forward/autoregressive pipeline |
| **Stage 5** | Weight Converter & `mmap` | `tools/convert_qwen.py`, `ai.cpp` | Sub-0.8s NVMe-to-VRAM weight streaming |
| **Stage 6** | C ABI DLL & ComfyUI Node | `axiom_c_api.cpp`, `Makefile`, `axiom_node.py` | `axiom_core.dll` bridged to ComfyUI via `ctypes` |
| **Stage 7** | Verification & Acceptance | `test_ai.cpp`, `test_inference.cpp` | All automated gates passing + live ComfyUI prompt expansion |

## Stage 1: Surgical Sync Eradication & Production VRAM Management

Before stacking multiple Transformer layers, you must eliminate the three hardware stalls identified in the codebase audit so multi-layer execution does not choke on PCIe barriers.

### Step 1.1: Eradicate Dynamic `cudaMalloc` in `core/src/CUDA/gpu/upload.cu`

Currently, `upload_to_gpu` checks buffer sizes at runtime and invokes `cudaMalloc`/`cudaFree`, triggering a global GPU synchronization barrier. Replace runtime allocations with a persistent, pre-allocated VRAM arena initialized once at startup.

1. Open `core/src/CUDA/util/gpu_types.cuh` and add fixed capacity tracking and async pool handles to `GPUPortion`:

```cpp
// Inside core/src/CUDA/util/gpu_types.cuh
#pragma once
#include <cuda_runtime.h>
#include <cuda_bf16.h>
#include <cstdint>

struct GPUPortion {
    cudaStream_t stream[2];
    cudaEvent_t  loss_ready_event[2];

    // Pre-allocated persistent device buffers per stream (Ping-Pong)
    uint32_t*       d_token_ids[2]      = {nullptr, nullptr};
    __nv_bfloat16*  d_context_window[2] = {nullptr, nullptr};
    __nv_bfloat16*  d_hidden_state[2]   = {nullptr, nullptr};
    float*          d_loss_scalar[2]    = {nullptr, nullptr};

    // Pinned host telemetry slots (4 bytes per stream)
    float*          h_pinned_loss[2]    = {nullptr, nullptr};

    size_t max_batch_tokens = 0; // Locked at initialization (e.g., M=32 * seq_len=128 = 4096)
    bool   is_allocated     = false;
};

```

2. Open `core/src/CUDA/gpu/upload.cu` and strip out the runtime `if (size != ...)` reallocation blocks. Allocate once during engine initialization and use `cudaMemcpyAsync` exclusively in the hot loop:

```cpp
// Inside core/src/CUDA/gpu/upload.cu
#include "../util/gpu_types.cuh"
#include <cstdio>

bool init_gpu_portion(GPUPortion* gpu, size_t max_batch_tokens, size_t hidden_dim) {
    if (gpu->is_allocated) return true;
    gpu->max_batch_tokens = max_batch_tokens;

    for (int s = 0; s < 2; ++s) {
        cudaStreamCreateWithFlags(&gpu->stream[s], cudaStreamNonBlocking);
        cudaEventCreateWithFlags(&gpu->loss_ready_event[s], cudaEventDisableTiming);

        cudaMalloc(&gpu->d_token_ids[s], max_batch_tokens * sizeof(uint32_t));
        cudaMalloc(&gpu->d_context_window[s], max_batch_tokens * hidden_dim * sizeof(__nv_bfloat16));
        cudaMalloc(&gpu->d_hidden_state[s], max_batch_tokens * hidden_dim * sizeof(__nv_bfloat16));
        cudaMalloc(&gpu->d_loss_scalar[s], sizeof(float));

        cudaHostAlloc(&gpu->h_pinned_loss[s], sizeof(float), cudaHostAllocPortable);
        *(gpu->h_pinned_loss[s]) = 0.0f;
    }
    gpu->is_allocated = true;
    return true;
}

void upload_tokens_async(GPUPortion* gpu, int stream_idx, const uint32_t* h_pinned_tokens, size_t count) {
    // Zero cudaMalloc calls in the hot path
    cudaMemcpyAsync(
        gpu->d_token_ids[stream_idx],
        h_pinned_tokens,
        count * sizeof(uint32_t),
        cudaMemcpyHostToDevice,
        gpu->stream[stream_idx]
    );
}

```

### Step 1.2: Non-Blocking Loss Telemetry in `core/src/CUDA/gpu/download.cu`

Replace the synchronous `cudaDeviceSynchronize()` call inside `download_loss` with an asynchronous event query so Stream 0 and Stream 1 never stall while reporting loss to the host CLI:

```cpp
// Inside core/src/CUDA/gpu/download.cu
#include "../util/gpu_types.cuh"

void enqueue_loss_download(GPUPortion* gpu, int stream_idx) {
    cudaMemcpyAsync(
        gpu->h_pinned_loss[stream_idx],
        gpu->d_loss_scalar[stream_idx],
        sizeof(float),
        cudaMemcpyDeviceToHost,
        gpu->stream[stream_idx]
    );
    cudaEventRecord(gpu->loss_ready_event[stream_idx], gpu->stream[stream_idx]);
}

bool poll_loss_nonblocking(GPUPortion* gpu, int stream_idx, float& out_loss) {
    cudaError_t status = cudaEventQuery(gpu->loss_ready_event[stream_idx]);
    if (status == cudaSuccess) {
        out_loss = *(gpu->h_pinned_loss[stream_idx]);
        return true; // New loss value ready without blocking GPU compute
    }
    return false; // Still computing; keep running
}

```

### Step 1.3: Lock-Free SPSC Ring Buffer in `core/include/optimizer.hpp` & `core/util/optimizer.cpp`

Replace the `while (buffer_in_use)` spin-wait loop with a cache-line-aligned Single-Producer Single-Consumer (SPSC) lock-free ring buffer:

```cpp
// Inside core/include/optimizer.hpp
#pragma once
#include <atomic>
#include <cstdint>
#include <cstddef>

template <typename T, size_t Capacity>
class LockFreeSPSCRingBuffer {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");
private:
    alignas(64) std::atomic<size_t> write_idx_{0};
    alignas(64) std::atomic<size_t> read_idx_{0};
    alignas(64) T buffer_[Capacity];

public:
    bool try_push(const T& item) noexcept {
        const size_t w = write_idx_.load(std::memory_order_relaxed);
        const size_t next_w = (w + 1) & (Capacity - 1);
        if (next_w == read_idx_.load(std::memory_order_acquire)) {
            return false; // Buffer full
        }
        buffer_[w] = item;
        write_idx_.store(next_w, std::memory_order_release);
        return true;
    }

    bool try_pop(T& out_item) noexcept {
        const size_t r = read_idx_.load(std::memory_order_relaxed);
        if (r == write_idx_.load(std::memory_order_acquire)) {
            return false; // Buffer empty
        }
        out_item = buffer_[r];
        read_idx_.store((r + 1) & (Capacity - 1), std::memory_order_release);
        return true;
    }
};

```

## Stage 2: Domain Tokenizer Injection & Batched 2D Dispatch ($M = 32$)

### Step 2.1: Inject Reserved x86_64 & Structural Tokens (`core/util/parser.cpp`)

To prevent opcodes and registers from splintering into 4–8 subword tokens during Axiom v1 training, pre-seed IDs `4` through `515` at dictionary initialization:

```cpp
// Inside core/util/parser.cpp -> Parser::init_reserved_vocabulary()
void Parser::init_reserved_vocabulary() {
    static const char* kX86Primitives[] = {
        // 64-bit General Purpose Registers
        "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp",
        "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
        // Vector & SIMD Registers
        "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7",
        "ymm0", "ymm1", "ymm2", "ymm3", "zmm0", "zmm1", "zmm2", "zmm3",
        // High-frequency x86_64 / AVX2 / FMA Opcodes & Directives
        "mov", "lea", "add", "sub", "imul", "idiv", "xor", "and", "or", "shl", "shr",
        "cmp", "test", "je", "jne", "jg", "jl", "jmp", "call", "ret", "push", "pop",
        "vmovups", "vmovaps", "vfmadd231ps", "vbroadcastss", "vxorps", "vaddps",
        "section", ".text", ".data", "global", "[intent]", "[var]"
    };

    uint32_t next_id = 4; // 0=PAD, 1=UNK, 2=BOS, 3=EOS
    for (const char* token_str : kX86Primitives) {
        main_dictionary[token_str] = next_id;
        decode_ids[next_id] = token_str;
        ++next_id;
    }
}

```

### Step 2.2: Upgrade Gather Kernel & CUTLASS GEMM to Batched 2D Grid ($M = 32$)

Single-sequence dispatch ($M=1$) leaves 93% of an Ada Lovelace Tensor Core warp tile ($16 \times 16 \times 16$) idle. Upgrading `gather_embeddings_kernel` in `core/src/CUDA/kernel/forward_pass.cu` to a 2D grid processes $B \times S$ tokens in parallel:

```cpp
// Inside core/src/CUDA/kernel/forward_pass.cu
__global__ void gather_embeddings_batched_kernel(
    const uint32_t* __restrict__ d_token_ids,      // [B * S]
    const __nv_bfloat16* __restrict__ d_table,     // [V, D]
    __nv_bfloat16* __restrict__ d_output,          // [B * S, D]
    uint32_t total_tokens,                         // M = B * S (e.g., 32 * 128 = 4096)
    uint32_t hidden_dim,                           // D (e.g., 32, 512, or 1536)
    uint32_t vocab_size                            // V = 65536 or 151936
) {
    uint32_t token_idx = blockIdx.y * blockDim.y + threadIdx.y;
    uint32_t dim_idx   = blockIdx.x * blockDim.x + threadIdx.x;

    if (token_idx < total_tokens && dim_idx < hidden_dim) {
        uint32_t id = d_token_ids[token_idx];
        uint32_t safe_id = (id < vocab_size) ? id : 1u; // Clamp out-of-bounds to <UNK> (1)
        d_output[token_idx * hidden_dim + dim_idx] = d_table[safe_id * hidden_dim + dim_idx];
    }
}

void launch_gather_embeddings_batched(
    const uint32_t* d_token_ids,
    const __nv_bfloat16* d_table,
    __nv_bfloat16* d_output,
    uint32_t batch_tokens,
    uint32_t hidden_dim,
    uint32_t vocab_size,
    cudaStream_t stream
) {
    dim3 threads(32, 8); // 256 threads per block
    dim3 blocks((hidden_dim + threads.x - 1) / threads.x,
                (batch_tokens + threads.y - 1) / threads.y);
    gather_embeddings_batched_kernel<<<blocks, threads, 0, stream>>>(
        d_token_ids, d_table, d_output, batch_tokens, hidden_dim, vocab_size
    );
}

```

When passing `d_output` into your CUTLASS v4.8.0 GEMM adapter in `forward_pass.cu`, set the problem size `M = batch_tokens` (e.g., `M = 4096` during training or `M = seq_len` during prefill), `N = out_features`, and `K = hidden_dim`.

## Stage 3: Implement Missing Transformer & Qwen CUDA Primitives

Create the four modular CUDA kernels required by both Axiom v1's stacked architecture and Qwen2.5 (`rmsnorm.cu`, `rope.cu`, `swiglu.cu`, and `paged_gqa.cu`).

### Step 3.1: Warp-Reduced RMSNorm Kernel (`core/src/CUDA/kernel/rmsnorm.cu`)

Implements $y_i = \frac{x_i}{\sqrt{\frac{1}{D}\sum_{j=0}^{D-1} x_j^2 + \epsilon}} \cdot \gamma_i$ using register-level `__shfl_xor_sync` reductions:

```cpp
// File: core/src/CUDA/kernel/rmsnorm.cu
#include <cuda_runtime.h>
#include <cuda_bf16.h>

__inline__ __device__ float warp_reduce_sum(float val) {
    #pragma unroll
    for (int offset = 16; offset > 0; offset >>= 1) {
        val += __shfl_xor_sync(0xffffffff, val, offset);
    }
    return val;
}

__global__ void rmsnorm_bf16_kernel(
    const __nv_bfloat16* __restrict__ d_in,     // [M, D]
    const __nv_bfloat16* __restrict__ d_weight, // [D]
    __nv_bfloat16* __restrict__ d_out,          // [M, D]
    int hidden_dim,
    float eps
) {
    int row = blockIdx.x;
    int tid = threadIdx.x;

    const __nv_bfloat16* row_in = d_in + row * hidden_dim;
    __nv_bfloat16* row_out      = d_out + row * hidden_dim;

    float sum_sq = 0.0f;
    for (int i = tid; i < hidden_dim; i += blockDim.x) {
        float v = __bfloat162float(row_in[i]);
        sum_sq += v * v;
    }

    sum_sq = warp_reduce_sum(sum_sq);

    __shared__ float s_warp_sums[32];
    int lane = tid & 31;
    int wid  = tid >> 5;

    if (lane == 0) s_warp_sums[wid] = sum_sq;
    __syncthreads();

    if (wid == 0) {
        float block_sum = (tid < (blockDim.x >> 5)) ? s_warp_sums[lane] : 0.0f;
        block_sum = warp_reduce_sum(block_sum);
        if (tid == 0) {
            s_warp_sums[0] = rsqrtf(block_sum / static_cast<float>(hidden_dim) + eps);
        }
    }
    __syncthreads();

    float inv_rms = s_warp_sums[0];

    for (int i = tid; i < hidden_dim; i += blockDim.x) {
        float v = __bfloat162float(row_in[i]);
        float w = __bfloat162float(d_weight[i]);
        row_out[i] = __float2bfloat16(v * inv_rms * w);
    }
}

void launch_rmsnorm_bf16(
    const __nv_bfloat16* d_in,
    const __nv_bfloat16* d_weight,
    __nv_bfloat16* d_out,
    int num_tokens,
    int hidden_dim,
    float eps,
    cudaStream_t stream
) {
    int threads = (hidden_dim >= 512) ? 256 : 128;
    rmsnorm_bf16_kernel<<<num_tokens, threads, 0, stream>>>(
        d_in, d_weight, d_out, hidden_dim, eps
    );
}

```

### Step 3.2: Rotary Positional Embeddings (RoPE) Kernel (`core/src/CUDA/kernel/rope.cu`)

Applies half-dim complex plane rotation (standard across Qwen2.5 and LLaMA-style models where pairs are $(i, i + d_{\text{head}}/2)$) with base frequency $\theta_{\text{base}} = 1{,}000{,}000.0$:

```cpp
// File: core/src/CUDA/kernel/rope.cu
#include <cuda_runtime.h>
#include <cuda_bf16.h>
#include <cmath>
#include <cstdint>

__global__ void rope_inplace_bf16_kernel(
    __nv_bfloat16* __restrict__ d_q, // [num_tokens, num_q_heads * head_dim]
    __nv_bfloat16* __restrict__ d_k, // [num_tokens, num_kv_heads * head_dim]
    const int32_t* __restrict__ d_pos, // [num_tokens] logical token positions
    int num_tokens,
    int num_q_heads,
    int num_kv_heads,
    int head_dim,
    float rope_theta
) {
    int token_idx = blockIdx.x;
    if (token_idx >= num_tokens) return;

    int half_dim = head_dim >> 1;
    int pos = d_pos[token_idx];

    // Rotate Q heads and K heads in a single pass
    int total_pairs = (num_q_heads + num_kv_heads) * half_dim;
    for (int idx = threadIdx.x; idx < total_pairs; idx += blockDim.x) {
        int head_idx = idx / half_dim;
        int pair_idx = idx % half_dim;

        float exponent = static_cast<float>(2 * pair_idx) / static_cast<float>(head_dim);
        float freq = 1.0f / powf(rope_theta, exponent);
        float angle = static_cast<float>(pos) * freq;
        float cos_val, sin_val;
        __sincosf(angle, &sin_val, &cos_val);

        if (head_idx < num_q_heads) {
            __nv_bfloat16* q_head = d_q + token_idx * (num_q_heads * head_dim) + head_idx * head_dim;
            float q0 = __bfloat162float(q_head[pair_idx]);
            float q1 = __bfloat162float(q_head[pair_idx + half_dim]);
            q_head[pair_idx]            = __float2bfloat16(q0 * cos_val - q1 * sin_val);
            q_head[pair_idx + half_dim] = __float2bfloat16(q1 * cos_val + q0 * sin_val);
        } else {
            int kv_head_idx = head_idx - num_q_heads;
            __nv_bfloat16* k_head = d_k + token_idx * (num_kv_heads * head_dim) + kv_head_idx * head_dim;
            float k0 = __bfloat162float(k_head[pair_idx]);
            float k1 = __bfloat162float(k_head[pair_idx + half_dim]);
            k_head[pair_idx]            = __float2bfloat16(k0 * cos_val - k1 * sin_val);
            k_head[pair_idx + half_dim] = __float2bfloat16(k1 * cos_val + k0 * sin_val);
        }
    }
}

void launch_rope_inplace_bf16(
    __nv_bfloat16* d_q,
    __nv_bfloat16* d_k,
    const int32_t* d_pos,
    int num_tokens,
    int num_q_heads,
    int num_kv_heads,
    int head_dim,
    float rope_theta,
    cudaStream_t stream
) {
    rope_inplace_bf16_kernel<<<num_tokens, 256, 0, stream>>>(
        d_q, d_k, d_pos, num_tokens, num_q_heads, num_kv_heads, head_dim, rope_theta
    );
}

```

### Step 3.3: Fused SwiGLU Activation Kernel (`core/src/CUDA/kernel/swiglu.cu`)

Computes $\text{SwiGLU}(g, u) = \left(\frac{g}{1 + e^{-g}}\right) \cdot u$ in a single VRAM pass:

```cpp
// File: core/src/CUDA/kernel/swiglu.cu
#include <cuda_runtime.h>
#include <cuda_bf16.h>
#include <cmath>

__global__ void swiglu_bf16_kernel(
    const __nv_bfloat16* __restrict__ d_gate, // [M, ffn_dim]
    const __nv_bfloat16* __restrict__ d_up,   // [M, ffn_dim]
    __nv_bfloat16* __restrict__ d_out,        // [M, ffn_dim]
    size_t total_elements
) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < total_elements) {
        float g = __bfloat162float(d_gate[idx]);
        float u = __bfloat162float(d_up[idx]);
        float silu_g = g / (1.0f + expf(-g));
        d_out[idx] = __float2bfloat16(silu_g * u);
    }
}

void launch_swiglu_bf16(
    const __nv_bfloat16* d_gate,
    const __nv_bfloat16* d_up,
    __nv_bfloat16* d_out,
    size_t total_elements,
    cudaStream_t stream
) {
    int threads = 256;
    int blocks = static_cast<int>((total_elements + threads - 1) / threads);
    swiglu_bf16_kernel<<<blocks, threads, 0, stream>>>(d_gate, d_up, d_out, total_elements);
}

```

### Step 3.4: Grouped Query Attention (GQA) + Paged KV-Cache Kernel (`core/src/CUDA/kernel/paged_gqa.cu`)

Maps Query head $h_q$ to shared Key/Value head $h_{kv} = \lfloor h_q / (N_q / N_{kv}) \rfloor$ across 16-token PagedAttention physical blocks:

```cpp
// File: core/src/CUDA/kernel/paged_gqa.cu
#include <cuda_runtime.h>
#include <cuda_bf16.h>
#include <cmath>
#include <cstdint>

constexpr int KV_BLOCK_SIZE = 16;

// Appends current step's K and V vectors into the physical KV page table
__global__ void append_paged_kv_kernel(
    const __nv_bfloat16* __restrict__ d_k,     // [num_kv_heads, head_dim]
    const __nv_bfloat16* __restrict__ d_v,     // [num_kv_heads, head_dim]
    __nv_bfloat16* __restrict__ d_k_cache,     // [num_blocks, KV_BLOCK_SIZE, num_kv_heads, head_dim]
    __nv_bfloat16* __restrict__ d_v_cache,     // [num_blocks, KV_BLOCK_SIZE, num_kv_heads, head_dim]
    const int32_t* __restrict__ d_block_table, // [max_blocks_per_seq]
    int token_pos,
    int num_kv_heads,
    int head_dim
) {
    int kv_head = blockIdx.x;
    int d       = threadIdx.x;
    if (kv_head >= num_kv_heads || d >= head_dim) return;

    int logical_block = token_pos / KV_BLOCK_SIZE;
    int block_offset  = token_pos % KV_BLOCK_SIZE;
    int physical_block = d_block_table[logical_block];

    size_t cache_idx = ((static_cast<size_t>(physical_block) * KV_BLOCK_SIZE + block_offset) * num_kv_heads + kv_head) * head_dim + d;
    size_t src_idx   = static_cast<size_t>(kv_head) * head_dim + d;

    d_k_cache[cache_idx] = d_k[src_idx];
    d_v_cache[cache_idx] = d_v[src_idx];
}

// Evaluates causal GQA over Paged KV-Cache for autoregressive decoding / prefill
__global__ void paged_gqa_decode_kernel(
    const __nv_bfloat16* __restrict__ d_q,     // [num_q_heads, head_dim]
    const __nv_bfloat16* __restrict__ d_k_cache,
    const __nv_bfloat16* __restrict__ d_v_cache,
    const int32_t* __restrict__ d_block_table,
    __nv_bfloat16* __restrict__ d_attn_out,    // [num_q_heads, head_dim]
    int seq_len,                               // Current total length (token_pos + 1)
    int num_q_heads,
    int num_kv_heads,
    int head_dim,
    float scale                                // 1.0f / sqrtf(head_dim)
) {
    extern __shared__ float s_logits[]; // Dynamic shared memory [seq_len]
    int q_head  = blockIdx.x;
    int tid     = threadIdx.x;
    int gqa_ratio = num_q_heads / num_kv_heads;
    int kv_head = q_head / gqa_ratio;

    const __nv_bfloat16* q_vec = d_q + q_head * head_dim;

    // 1. Compute Q * K^T across all cached positions t = 0 .. seq_len - 1
    float local_max = -1e30f;
    for (int t = tid; t < seq_len; t += blockDim.x) {
        int phys_block = d_block_table[t / KV_BLOCK_SIZE];
        int offset     = t % KV_BLOCK_SIZE;
        const __nv_bfloat16* k_vec = d_k_cache + ((static_cast<size_t>(phys_block) * KV_BLOCK_SIZE + offset) * num_kv_heads + kv_head) * head_dim;

        float dot = 0.0f;
        for (int d = 0; d < head_dim; ++d) {
            dot += __bfloat162float(q_vec[d]) * __bfloat162float(k_vec[d]);
        }
        float score = dot * scale;
        s_logits[t] = score;
        if (score > local_max) local_max = score;
    }

    // Block-wide max reduction for numerical stability
    __shared__ float s_reduce[256];
    s_reduce[tid] = local_max;
    __syncthreads();
    for (int s = blockDim.x >> 1; s > 0; s >>= 1) {
        if (tid < s && s_reduce[tid + s] > s_reduce[tid]) {
            s_reduce[tid] = s_reduce[tid + s];
        }
        __syncthreads();
    }
    float max_score = s_reduce[0];

    // 2. Exponentiate and sum
    float local_sum = 0.0f;
    for (int t = tid; t < seq_len; t += blockDim.x) {
        float p = expf(s_logits[t] - max_score);
        s_logits[t] = p;
        local_sum += p;
    }
    s_reduce[tid] = local_sum;
    __syncthreads();
    for (int s = blockDim.x >> 1; s > 0; s >>= 1) {
        if (tid < s) s_reduce[tid] += s_reduce[tid + s];
        __syncthreads();
    }
    float inv_sum = 1.0f / (s_reduce[0] + 1e-8f);

    for (int t = tid; t < seq_len; t += blockDim.x) {
        s_logits[t] *= inv_sum;
    }
    __syncthreads();

    // 3. Weighted sum over V cache along head_dim
    for (int d = tid; d < head_dim; d += blockDim.x) {
        float acc = 0.0f;
        for (int t = 0; t < seq_len; ++t) {
            int phys_block = d_block_table[t / KV_BLOCK_SIZE];
            int offset     = t % KV_BLOCK_SIZE;
            const __nv_bfloat16* v_vec = d_v_cache + ((static_cast<size_t>(phys_block) * KV_BLOCK_SIZE + offset) * num_kv_heads + kv_head) * head_dim;
            acc += s_logits[t] * __bfloat162float(v_vec[d]);
        }
        d_attn_out[q_head * head_dim + d] = __float2bfloat16(acc);
    }
}

```

## Stage 4: Multi-Layer Stacked Execution Pipeline (`ai.hpp` & `ai.cpp`)

### Step 4.1: Define the Multi-Layer Configuration & Weight Pointer Table

In `core/include/ai.hpp`, define `AxiomModelConfig` and `TransformerLayerWeights` so the engine can run either your 6-layer native Axiom v1 model or a 28-layer Qwen2.5-1.5B checkpoint simply by reading the header:

```cpp
// Inside core/include/ai.hpp
#pragma once
#include <cuda_bf16.h>
#include <cstdint>

struct AxiomModelConfig {
    uint32_t magic          = 0x4D4F4958; // "XIOM"
    uint32_t version        = 2;
    uint32_t num_layers     = 28;         // 6 for Axiom v1, 28 for Qwen2.5-1.5B
    uint32_t hidden_dim     = 1536;       // 512 for Axiom v1, 1536 for Qwen2.5-1.5B
    uint32_t num_q_heads    = 12;         // 8 for Axiom v1, 12 for Qwen2.5-1.5B
    uint32_t num_kv_heads   = 2;          // 2 for Qwen2.5-1.5B GQA
    uint32_t head_dim       = 128;        // 1536 / 12 = 128
    uint32_t ffn_dim        = 8960;       // SwiGLU intermediate size
    uint32_t vocab_size     = 151936;     // 65536 for Axiom v1, 151936 for Qwen2.5
    uint32_t max_seq_len    = 4096;
    float    rms_norm_eps   = 1e-6f;
    float    rope_theta     = 1000000.0f;
};

struct TransformerLayerWeights {
    const __nv_bfloat16* d_input_layernorm;     // [hidden_dim]
    const __nv_bfloat16* d_q_proj_weight;       // [num_q_heads * head_dim, hidden_dim]
    const __nv_bfloat16* d_q_proj_bias;         // [num_q_heads * head_dim] (Qwen uses QKV bias)
    const __nv_bfloat16* d_k_proj_weight;       // [num_kv_heads * head_dim, hidden_dim]
    const __nv_bfloat16* d_k_proj_bias;         // [num_kv_heads * head_dim]
    const __nv_bfloat16* d_v_proj_weight;       // [num_kv_heads * head_dim, hidden_dim]
    const __nv_bfloat16* d_v_proj_bias;         // [num_kv_heads * head_dim]
    const __nv_bfloat16* d_o_proj_weight;       // [hidden_dim, num_q_heads * head_dim]
    const __nv_bfloat16* d_post_attn_layernorm; // [hidden_dim]
    const __nv_bfloat16* d_gate_proj_weight;    // [ffn_dim, hidden_dim]
    const __nv_bfloat16* d_up_proj_weight;      // [ffn_dim, hidden_dim]
    const __nv_bfloat16* d_down_proj_weight;    // [hidden_dim, ffn_dim]
};

```

### Step 4.2: Implement the Sequential Multi-Layer Loop in `core/src/ai.cpp`

Chain the layers $l = 0 \dots N-1$ inside `AI::forward_transformer_step`:

```cpp
// Inside core/src/ai.cpp
void AI::forward_transformer_step(int token_id, int token_pos, float* d_out_logits, cudaStream_t stream) {
    const uint32_t D   = config_.hidden_dim;
    const uint32_t Qd  = config_.num_q_heads * config_.head_dim;
    const uint32_t KVd = config_.num_kv_heads * config_.head_dim;
    const uint32_t FFN = config_.ffn_dim;

    // 1. Copy single token ID & position and gather [1, D] embedding
    cudaMemcpyAsync(d_curr_token_, &token_id, sizeof(uint32_t), cudaMemcpyHostToDevice, stream);
    cudaMemcpyAsync(d_curr_pos_, &token_pos, sizeof(int32_t), cudaMemcpyHostToDevice, stream);

    launch_gather_embeddings_batched(
        d_curr_token_, d_embed_tokens_, d_residual_stream_,
        1, D, config_.vocab_size, stream
    );

    // 2. Loop sequentially through all N Transformer blocks
    for (uint32_t l = 0; l < config_.num_layers; ++l) {
        const auto& w = layers_[l];

        // 2a. Pre-Attention RMSNorm
        launch_rmsnorm_bf16(d_residual_stream_, w.d_input_layernorm, d_normed_x_, 1, D, config_.rms_norm_eps, stream);

        // 2b. Q, K, V Projections via CUTLASS Linear Identity EVT (Y = X * W^T + bias)
        launch_cutlass_gemm_bf16(1, Qd,  D, d_normed_x_, w.d_q_proj_weight, w.d_q_proj_bias, d_q_, stream);
        launch_cutlass_gemm_bf16(1, KVd, D, d_normed_x_, w.d_k_proj_weight, w.d_k_proj_bias, d_k_, stream);
        launch_cutlass_gemm_bf16(1, KVd, D, d_normed_x_, w.d_v_proj_weight, w.d_v_proj_bias, d_v_, stream);

        // 2c. In-place RoPE on Q and K
        launch_rope_inplace_bf16(
            d_q_, d_k_, d_curr_pos_, 1,
            config_.num_q_heads, config_.num_kv_heads, config_.head_dim, config_.rope_theta, stream
        );

        // 2d. Append K, V to layer l's Paged KV-Cache & execute Paged GQA
        append_paged_kv_kernel<<<config_.num_kv_heads, config_.head_dim, 0, stream>>>(
            d_k_, d_v_, d_k_cache_[l], d_v_cache_[l], d_block_table_,
            token_pos, config_.num_kv_heads, config_.head_dim
        );

        float scale = 1.0f / std::sqrt(static_cast<float>(config_.head_dim));
        size_t smem_bytes = (token_pos + 1) * sizeof(float);
        paged_gqa_decode_kernel<<<config_.num_q_heads, 128, smem_bytes, stream>>>(
            d_q_, d_k_cache_[l], d_v_cache_[l], d_block_table_, d_attn_out_,
            token_pos + 1, config_.num_q_heads, config_.num_kv_heads, config_.head_dim, scale
        );

        // 2e. Output Projection + Residual Add
        launch_cutlass_gemm_bf16(1, D, Qd, d_attn_out_, w.d_o_proj_weight, nullptr, d_proj_tmp_, stream);
        launch_residual_add_bf16(d_residual_stream_, d_proj_tmp_, D, stream);

        // 2f. Pre-FFN RMSNorm
        launch_rmsnorm_bf16(d_residual_stream_, w.d_post_attn_layernorm, d_normed_x_, 1, D, config_.rms_norm_eps, stream);

        // 2g. SwiGLU FFN: Gate & Up GEMMs -> SwiGLU -> Down GEMM -> Residual Add
        launch_cutlass_gemm_bf16(1, FFN, D, d_normed_x_, w.d_gate_proj_weight, nullptr, d_gate_buf_, stream);
        launch_cutlass_gemm_bf16(1, FFN, D, d_normed_x_, w.d_up_proj_weight,   nullptr, d_up_buf_,   stream);
        launch_swiglu_bf16(d_gate_buf_, d_up_buf_, d_act_buf_, FFN, stream);
        launch_cutlass_gemm_bf16(1, D, FFN, d_act_buf_,  w.d_down_proj_weight, nullptr, d_proj_tmp_, stream);
        launch_residual_add_bf16(d_residual_stream_, d_proj_tmp_, D, stream);
    }

    // 3. Final RMSNorm & LM Head Logit Projection
    launch_rmsnorm_bf16(d_residual_stream_, d_final_norm_weight_, d_normed_x_, 1, D, config_.rms_norm_eps, stream);
    launch_tied_logits_bf16(d_normed_x_, d_lm_head_weight_, d_out_logits, 1, config_.vocab_size, D, stream);
}

```

## Stage 5: Weight Ingestion (`convert_qwen.py`) & Fast Win32 `mmap` Loader

### Step 5.1: Create the Offline Weight Converter (`tools/convert_qwen.py`)

This standalone script runs once on CPU to pack official HuggingFace `model.safetensors` weights into a single flat binary (`models/qwen1_5b.axiom`) with exact tensor order matching `TransformerLayerWeights`:

```python
# File: tools/convert_qwen.py
import struct
import sys
from pathlib import Path
from safetensors import safe_open
import torch

def export_qwen_to_axiom(safetensors_path: str, output_axiom_path: str):
    print(f"[Axiom Converter] Opening {safetensors_path}...")
    f = safe_open(safetensors_path, framework="pt", device="cpu")

    # Qwen2.5-1.5B architectural constants
    magic        = 0x4D4F4958 # "XIOM"
    version      = 2
    num_layers   = 28
    hidden_dim   = 1536
    num_q_heads  = 12
    num_kv_heads = 2
    head_dim     = 128
    ffn_dim      = 8960
    vocab_size   = 151936
    max_seq_len  = 4096
    rms_eps      = 1e-6
    rope_theta   = 1000000.0

    header = struct.pack(
        "<10I2f16x", # 64-byte aligned header (48 bytes payload + 16 pad bytes)
        magic, version, num_layers, hidden_dim, num_q_heads,
        num_kv_heads, head_dim, ffn_dim, vocab_size, max_seq_len,
        rms_eps, rope_theta
    )

    def write_bf16(out_file, tensor_name: str):
        t = f.get_tensor(tensor_name).to(torch.bfloat16).contiguous()
        raw_bytes = t.view(torch.uint16).numpy().tobytes()
        out_file.write(raw_bytes)

    with open(output_axiom_path, "wb") as out:
        out.write(header)
        # 1. Token embedding table
        write_bf16(out, "model.embed_tokens.weight")

        # 2. Sequential layers 0 .. 27
        for l in range(num_layers):
            p = f"model.layers.{l}"
            write_bf16(out, f"{p}.input_layernorm.weight")
            write_bf16(out, f"{p}.self_attn.q_proj.weight")
            write_bf16(out, f"{p}.self_attn.q_proj.bias")
            write_bf16(out, f"{p}.self_attn.k_proj.weight")
            write_bf16(out, f"{p}.self_attn.k_proj.bias")
            write_bf16(out, f"{p}.self_attn.v_proj.weight")
            write_bf16(out, f"{p}.self_attn.v_proj.bias")
            write_bf16(out, f"{p}.self_attn.o_proj.weight")
            write_bf16(out, f"{p}.post_attention_layernorm.weight")
            write_bf16(out, f"{p}.mlp.gate_proj.weight")
            write_bf16(out, f"{p}.mlp.up_proj.weight")
            write_bf16(out, f"{p}.mlp.down_proj.weight")
            print(f"  -> Packed Layer {l+1}/{num_layers}")

        # 3. Final RMSNorm & LM Head (tied or explicit)
        write_bf16(out, "model.norm.weight")
        if "lm_head.weight" in f.keys():
            write_bf16(out, "lm_head.weight")
        else:
            write_bf16(out, "model.embed_tokens.weight")

    print(f"[Axiom Converter] Successfully wrote {output_axiom_path}")

if __name__ == "__main__":
    src = sys.argv[1] if len(sys.argv) > 1 else "model.safetensors"
    dst = sys.argv[2] if len(sys.argv) > 2 else "models/qwen1_5b.axiom"
    Path(dst).parent.mkdir(parents=True, exist_ok=True)
    export_qwen_to_axiom(src, dst)

```

### Step 5.2: Win32 Memory-Mapped Zero-Copy VRAM Loader (`core/src/ai.cpp`)

Stream the entire `.axiom` binary from NVMe SSD straight into a single contiguous VRAM block on your RTX 4080 and assign layer pointers by offset arithmetic:

```cpp
// Inside core/src/ai.cpp
#include <windows.h>

bool AI::load_axiom_model_mmap(const char* filepath) {
    HANDLE hFile = CreateFileA(
        filepath, GENERIC_READ, FILE_SHARE_READ, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr
    );
    if (hFile == INVALID_HANDLE_VALUE) return false;

    LARGE_INTEGER file_size;
    GetFileSizeEx(hFile, &file_size);
    size_t total_bytes = static_cast<size_t>(file_size.QuadPart);

    HANDLE hMap = CreateFileMappingA(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
    const uint8_t* mapped_base = static_cast<const uint8_t*>(
        MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0)
    );

    // Read 64-byte header
    std::memcpy(&config_, mapped_base, sizeof(AxiomModelConfig));
    const uint8_t* host_weights_ptr = mapped_base + 64;
    size_t weights_bytes = total_bytes - 64;

    // Allocate single contiguous VRAM arena for all model weights
    cudaMalloc(&d_weights_arena_, weights_bytes);

    // Pin the OS file-mapped view for direct DMA transfer across PCIe
    cudaHostRegister(const_cast<uint8_t*>(host_weights_ptr), weights_bytes, cudaHostRegisterReadOnly);
    cudaMemcpyAsync(d_weights_arena_, host_weights_ptr, weights_bytes, cudaMemcpyHostToDevice, compute_stream_);
    cudaStreamSynchronize(compute_stream_);
    cudaHostUnregister(const_cast<uint8_t*>(host_weights_ptr));

    UnmapViewOfFile(mapped_base);
    CloseHandle(hMap);
    CloseHandle(hFile);

    // Slice contiguous VRAM arena into layer pointer offsets
    const __nv_bfloat16* cursor = static_cast<const __nv_bfloat16*>(d_weights_arena_);
    const size_t D   = config_.hidden_dim;
    const size_t Qd  = config_.num_q_heads * config_.head_dim;
    const size_t KVd = config_.num_kv_heads * config_.head_dim;
    const size_t FFN = config_.ffn_dim;
    const size_t V   = config_.vocab_size;

    d_embed_tokens_ = cursor; cursor += V * D;

    layers_.resize(config_.num_layers);
    for (uint32_t l = 0; l < config_.num_layers; ++l) {
        layers_[l].d_input_layernorm     = cursor; cursor += D;
        layers_[l].d_q_proj_weight       = cursor; cursor += Qd * D;
        layers_[l].d_q_proj_bias         = cursor; cursor += Qd;
        layers_[l].d_k_proj_weight       = cursor; cursor += KVd * D;
        layers_[l].d_k_proj_bias         = cursor; cursor += KVd;
        layers_[l].d_v_proj_weight       = cursor; cursor += KVd * D;
        layers_[l].d_v_proj_bias         = cursor; cursor += KVd;
        layers_[l].d_o_proj_weight       = cursor; cursor += D * Qd;
        layers_[l].d_post_attn_layernorm = cursor; cursor += D;
        layers_[l].d_gate_proj_weight    = cursor; cursor += FFN * D;
        layers_[l].d_up_proj_weight      = cursor; cursor += FFN * D;
        layers_[l].d_down_proj_weight    = cursor; cursor += D * FFN;
    }

    d_final_norm_weight_ = cursor; cursor += D;
    d_lm_head_weight_    = cursor;
    return true;
}

```

## Stage 6: Zero-PyTorch C ABI (`axiom_core.dll`) & ComfyUI Custom Node

### Step 6.1: Create the C Export Interface (`core/src/axiom_c_api.cpp`)

Expose a clean C ABI with zero C++ name-mangling so Python's built-in `ctypes` can call Axiom directly:

```cpp
// File: core/src/axiom_c_api.cpp
#include "../include/ai.hpp"
#include <cstring>
#include <memory>

#ifdef _WIN32
#define AXIOM_API extern "C" __declspec(dllexport)
#else
#define AXIOM_API extern "C" __attribute__((visibility("default")))
#endif

static std::unique_ptr<AI> g_axiom_engine = nullptr;

AXIOM_API bool axiom_load_model(const char* model_path, const char* vocab_path) {
    if (!g_axiom_engine) {
        g_axiom_engine = std::make_unique<AI>();
    }
    if (!g_axiom_engine->load_vocabulary(vocab_path)) return false;
    return g_axiom_engine->load_axiom_model_mmap(model_path);
}

AXIOM_API int axiom_generate_text(
    const char* prompt,
    int max_new_tokens,
    float temperature,
    int top_k,
    float top_p,
    char* out_buffer,
    int out_buffer_capacity
) {
    if (!g_axiom_engine || !prompt || !out_buffer || out_buffer_capacity <= 1) {
        return -1;
    }

    std::string accumulated;
    bool ok = g_axiom_engine->generate(
        std::string(prompt),
        static_cast<size_t>(max_new_tokens),
        temperature,
        top_k,
        top_p,
        [&accumulated](const std::string& token_str) {
            accumulated += token_str;
        }
    );

    if (!ok) return -2;

    size_t copy_len = accumulated.size();
    if (copy_len >= static_cast<size_t>(out_buffer_capacity)) {
        copy_len = static_cast<size_t>(out_buffer_capacity - 1);
    }
    std::memcpy(out_buffer, accumulated.data(), copy_len);
    out_buffer[copy_len] = '\0';
    return static_cast<int>(copy_len);
}

AXIOM_API void axiom_free_model() {
    g_axiom_engine.reset();
}

```

### Step 6.2: Add the Shared Library Target to `Makefile`

Add the `dll` target to your root `Makefile` to compile `build/axiom_core.dll` targeting `sm_89`:

```makefile
# Add inside root Makefile
DLL_TARGET := build/axiom_core.dll

dll: $(CUDA_OBJS) $(CORE_OBJS)
	$(NVCC) -arch=sm_89 -shared -o $(DLL_TARGET) \
		core/src/axiom_c_api.cpp $(CUDA_OBJS) $(CORE_OBJS) \
		-L"$(CUDA_PATH)/lib/x64" -lcudart -lcurand

```

### Step 6.3: Create the ComfyUI Node (`ComfyUI/custom_nodes/ComfyUI-ProjectAxiom/`)

Create two files inside your ComfyUI `custom_nodes/ComfyUI-ProjectAxiom` directory:

1. `ComfyUI/custom_nodes/ComfyUI-ProjectAxiom/axiom_node.py`:

```python
# File: ComfyUI/custom_nodes/ComfyUI-ProjectAxiom/axiom_node.py
import ctypes
import os
from pathlib import Path

class AxiomQwenNode:
    _dll = None
    _loaded_model_path = None

    @classmethod
    def INPUT_TYPES(cls):
        return {
            "required": {
                "prompt": ("STRING", {"multiline": True, "default": "Expand this into a detailed cinematic prompt:"}),
                "model_path": ("STRING", {"default": "C:/Users/Omid/My Work Place/Programming/PROJECT-AXIOM/models/qwen1_5b.axiom"}),
                "vocab_path": ("STRING", {"default": "C:/Users/Omid/My Work Place/Programming/PROJECT-AXIOM/models/vocab.axiom"}),
                "max_new_tokens": ("INT", {"default": 256, "min": 1, "max": 2048, "step": 1}),
                "temperature": ("FLOAT", {"default": 0.7, "min": 0.0, "max": 2.0, "step": 0.05}),
                "top_k": ("INT", {"default": 40, "min": 1, "max": 200, "step": 1}),
                "top_p": ("FLOAT", {"default": 0.9, "min": 0.1, "max": 1.0, "step": 0.05}),
            }
        }

    RETURN_TYPES = ("STRING",)
    RETURN_NAMES = ("expanded_prompt",)
    FUNCTION = "generate"
    CATEGORY = "Project Axiom"

    @classmethod
    def _ensure_dll(cls, model_path: str, vocab_path: str):
        if cls._dll is None:
            dll_path = Path(model_path).resolve().parent.parent / "build" / "axiom_core.dll"
            cls._dll = ctypes.CDLL(str(dll_path))

            cls._dll.axiom_load_model.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
            cls._dll.axiom_load_model.restype = ctypes.c_bool

            cls._dll.axiom_generate_text.argtypes = [
                ctypes.c_char_p, ctypes.c_int, ctypes.c_float,
                ctypes.c_int, ctypes.c_float, ctypes.c_char_p, ctypes.c_int
            ]
            cls._dll.axiom_generate_text.restype = ctypes.c_int

            cls._dll.axiom_free_model.argtypes = []
            cls._dll.axiom_free_model.restype = None

        if cls._loaded_model_path != model_path:
            ok = cls._dll.axiom_load_model(
                model_path.encode("utf-8"),
                vocab_path.encode("utf-8")
            )
            if not ok:
                raise RuntimeError(f"[Project Axiom] Failed to load model: {model_path}")
            cls._loaded_model_path = model_path

    def generate(self, prompt, model_path, vocab_path, max_new_tokens, temperature, top_k, top_p):
        self._ensure_dll(model_path, vocab_path)

        buf_size = max_new_tokens * 16 + 4096
        out_buf = ctypes.create_string_buffer(buf_size)

        bytes_written = self._dll.axiom_generate_text(
            prompt.encode("utf-8"),
            int(max_new_tokens),
            float(temperature),
            int(top_k),
            float(top_p),
            out_buf,
            buf_size
        )

        if bytes_written < 0:
            raise RuntimeError(f"[Project Axiom] Generation failed with error code {bytes_written}")

        return (out_buf.value.decode("utf-8", errors="replace"),)

```

2. `ComfyUI/custom_nodes/ComfyUI-ProjectAxiom/__init__.py`:

```python
# File: ComfyUI/custom_nodes/ComfyUI-ProjectAxiom/__init__.py
from .axiom_node import AxiomQwenNode

NODE_CLASS_MAPPINGS = {
    "AxiomQwenNode": AxiomQwenNode
}

NODE_DISPLAY_NAME_MAPPINGS = {
    "AxiomQwenNode": "Axiom Native C++/CUDA LLM (Zero-PyTorch)"
}

__all__ = ["NODE_CLASS_MAPPINGS", "NODE_DISPLAY_NAME_MAPPINGS"]

```

## Stage 7: Terminal Build & Verification Sequence

Run these commands in order from your Project Axiom root directory in PowerShell:

```powershell
# 1. Compile all new CUDA kernels (rmsnorm, rope, swiglu, paged_gqa) and run the 429+ test harness
mingw32-make clean
mingw32-make test

# 2. Verify zero runtime cudaMalloc / cudaDeviceSynchronize regressions
./build/test.exe --test

# 3. Build the standalone C ABI shared library for ComfyUI
mingw32-make dll

# 4. Convert official Qwen2.5-1.5B safetensors once into a flat .axiom binary
python tools/convert_qwen.py path/to/Qwen2.5-1.5B/model.safetensors models/qwen1_5b.axiom

# 5. Launch ComfyUI and wire [Axiom Native C++/CUDA LLM] -> [CLIP Text Encode (Prompt)]

```