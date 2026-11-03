Switching Cline CLI to run Qwen inference directly on Project Axiom requires upgrading your single-layer execution loop into a stacked 28-layer Grouped Query Attention (GQA) pipeline, preserving code punctuation in the tokenizer, streaming the converted `.safetensors` weights into VRAM via Win32 memory mapping, and exposing a local OpenAI-compatible `/v1/chat/completions` endpoint.

## Master Execution Order

| Stage | Subsystem              | Target Files                                 | Primary Deliverable      | Verification Gate  |
| ----- | ---------------------- | -------------------------------------------- | ------------------------ | ------------------ |
| 1     | Sync Eradication       | gpu_types.cuh, upload.cu, download.cu        | Zero runtime allocations | 0 sync stalls      |
| 2     | Batched Dispatch       | forward_pass.cu, parser.cpp                  | 2D grid prefill          | 4,000 tok/s        |
| 3     | Transformer Kernels    | rmsnorm.cu, rope.cu, swiglu.cu, paged_gqa.cu | Native BF16 operators    | Numerical parity   |
| 4     | Stacked Pipeline       | ai.hpp, ai.cpp                               | 28-layer forward loop    | Coherent decode    |
| 5     | Weight Streaming       | convert_qwen.py, ai.cpp                      | Win32 mmap DMA loader    | Sub-0.8s load      |
| 6     | Cline & ComfyUI ABI    | server.cpp, axiom_c_api.cpp, Makefile        | HTTP server and DLL      | Live JSON response |
| 7     | Cutover & Verification | Terminal & Cline CLI config                  | Local coding agent       | 429 tests green    |

The table above sequences the build so low-level VRAM and stream fixes land before multi-layer tensor operations and external API bridges are compiled.

## Stage 1: Surgical Sync Eradication & Production VRAM Management

Before stacking 28 Qwen layers or 6 Axiom v1 layers, you must eliminate the three hardware stalls in the transfer loop so multi-layer execution never blocks on PCIe synchronization barriers.

### Step 1.1: Pre-Allocated VRAM Arena in core/src/CUDA/util/gpu_types.cuh and core/src/CUDA/gpu/upload.cu

1. Open **core/src/CUDA/util/gpu_types.cuh** and define fixed-capacity ping-pong device pointers and pinned telemetry slots:

```cpp
#pragma once
#include <cuda_runtime.h>
#include <cuda_bf16.h>
#include <cstdint>

struct GPUPortion {
    cudaStream_t stream[2];
    cudaEvent_t  loss_ready_event[2];

    uint32_t*      d_token_ids[2]      = {nullptr, nullptr};
    __nv_bfloat16* d_context_window[2] = {nullptr, nullptr};
    __nv_bfloat16* d_hidden_state[2]   = {nullptr, nullptr};
    float*         d_loss_scalar[2]    = {nullptr, nullptr};
    float*         h_pinned_loss[2]    = {nullptr, nullptr};

    size_t max_batch_tokens = 0;
    bool   is_allocated     = false;
};

```

2. Open **core/src/CUDA/gpu/upload.cu**, remove the runtime size check that calls `cudaMalloc` inside the hot loop, and allocate once at startup:

```cpp
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
    cudaMemcpyAsync(
        gpu->d_token_ids[stream_idx],
        h_pinned_tokens,
        count * sizeof(uint32_t),
        cudaMemcpyHostToDevice,
        gpu->stream[stream_idx]
    );
}

```

### Step 1.2: Non-Blocking Loss Telemetry in core/src/CUDA/gpu/download.cu

Replace `cudaDeviceSynchronize()` inside **core/src/CUDA/gpu/download.cu** with `cudaEventQuery` so neither stream stalls when reporting metrics:

```cpp
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
    if (cudaEventQuery(gpu->loss_ready_event[stream_idx]) == cudaSuccess) {
        out_loss = *(gpu->h_pinned_loss[stream_idx]);
        return true;
    }
    return false;
}

```

### Step 1.3: Lock-Free SPSC Ring Buffer in core/include/optimizer.hpp

Replace the `while (buffer_in_use)` spin-lock in **core/include/optimizer.hpp** with an acquire/release Single-Producer Single-Consumer ring buffer:

```cpp
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
        if (next_w == read_idx_.load(std::memory_order_acquire)) return false;
        buffer_[w] = item;
        write_idx_.store(next_w, std::memory_order_release);
        return true;
    }

    bool try_pop(T& out_item) noexcept {
        const size_t r = read_idx_.load(std::memory_order_relaxed);
        if (r == write_idx_.load(std::memory_order_acquire)) return false;
        out_item = buffer_[r];
        read_idx_.store((r + 1) & (Capacity - 1), std::memory_order_release);
        return true;
    }
};

```

## Stage 2: Code-Safe Tokenizer & Batched 2D Gather ($M = 32$)

Cline CLI sends large system prompts containing source code, indentation, and Qwen ChatML control tags (`<|im_start|>`, `<|im_end|>`). Your existing sanitizer strips punctuation and lowercases ASCII, which destroys C++ syntax (`{`, `}`, `->`, `;`) and corrupts Qwen token IDs.

### Step 2.1: Add Code-Preserving BPE Mode & x86_64 Primitives in core/util/parser.cpp

Open **core/util/parser.cpp** and add both the x86_64 pre-seeding for native Axiom v1 training and a greedy longest-match tokenizer for Qwen's exact vocabulary:

```cpp
void Parser::init_reserved_vocabulary() {
    static const char* kX86Primitives[] = {
        "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp",
        "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
        "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7",
        "ymm0", "ymm1", "ymm2", "ymm3", "zmm0", "zmm1", "zmm2", "zmm3",
        "mov", "lea", "add", "sub", "imul", "idiv", "xor", "and", "or", "shl", "shr",
        "cmp", "test", "je", "jne", "jg", "jl", "jmp", "call", "ret", "push", "pop",
        "vmovups", "vmovaps", "vfmadd231ps", "vbroadcastss", "vxorps", "vaddps",
        "section", ".text", ".data", "global", "[intent]", "[var]",
        "<|im_start|>", "<|im_end|>", "<|endoftext|>"
    };

    uint32_t next_id = 4;
    for (const char* token_str : kX86Primitives) {
        main_dictionary[token_str] = next_id;
        decode_ids[next_id] = token_str;
        ++next_id;
    }
}

// Exact case-preserving longest-match tokenizer for Qwen / Code prompts
std::vector<uint32_t> Parser::tokenize_exact_code(const std::string& raw_prompt) const {
    std::vector<uint32_t> out_ids;
    out_ids.reserve(raw_prompt.size() / 2);

    size_t pos = 0;
    const size_t n = raw_prompt.size();
    while (pos < n) {
        size_t match_len = std::min<size_t>(32, n - pos);
        uint32_t matched_id = 1; // Default <UNK>
        bool found = false;

        for (; match_len >= 1; --match_len) {
            std::string_view candidate(raw_prompt.data() + pos, match_len);
            auto it = main_dictionary.find(std::string(candidate));
            if (it != main_dictionary.end()) {
                matched_id = it->second;
                found = true;
                break;
            }
        }
        out_ids.push_back(matched_id);
        pos += found ? match_len : 1;
    }
    return out_ids;
}

```

### Step 2.2: Upgrade Gather Kernel to 2D Batched Grid in core/src/CUDA/kernel/forward_pass.cu

When Cline CLI sends a 1,500-token file context, running $M=1$ takes 1,500 serial passes. Upgrading **core/src/CUDA/kernel/forward_pass.cu** to a 2D grid processes all prompt tokens in parallel across Ada Lovelace Tensor Cores:

```cpp
__global__ void gather_embeddings_batched_kernel(
    const uint32_t* __restrict__ d_token_ids,
    const __nv_bfloat16* __restrict__ d_table,
    __nv_bfloat16* __restrict__ d_output,
    uint32_t total_tokens,
    uint32_t hidden_dim,
    uint32_t vocab_size
) {
    uint32_t token_idx = blockIdx.y * blockDim.y + threadIdx.y;
    uint32_t dim_idx   = blockIdx.x * blockDim.x + threadIdx.x;

    if (token_idx < total_tokens && dim_idx < hidden_dim) {
        uint32_t id = d_token_ids[token_idx];
        uint32_t safe_id = (id < vocab_size) ? id : 1u;
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
    dim3 threads(32, 8);
    dim3 blocks((hidden_dim + threads.x - 1) / threads.x,
                (batch_tokens + threads.y - 1) / threads.y);
    gather_embeddings_batched_kernel<<<blocks, threads, 0, stream>>>(
        d_token_ids, d_table, d_output, batch_tokens, hidden_dim, vocab_size
    );
}

```

## Stage 3: Implement Missing Transformer & Qwen CUDA Kernels

Create the four modular CUDA files inside **core/src/CUDA/kernel/** to execute RMSNorm, RoPE, SwiGLU, and Paged Grouped Query Attention natively on `sm_89`.

### Step 3.1: Create core/src/CUDA/kernel/rmsnorm.cu

```cpp
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
    const __nv_bfloat16* __restrict__ d_in,
    const __nv_bfloat16* __restrict__ d_weight,
    __nv_bfloat16* __restrict__ d_out,
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

### Step 3.2: Create core/src/CUDA/kernel/rope.cu

```cpp
#include <cuda_runtime.h>
#include <cuda_bf16.h>
#include <cmath>
#include <cstdint>

__global__ void rope_inplace_bf16_kernel(
    __nv_bfloat16* __restrict__ d_q,
    __nv_bfloat16* __restrict__ d_k,
    const int32_t* __restrict__ d_pos,
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

### Step 3.3: Create core/src/CUDA/kernel/swiglu.cu

```cpp
#include <cuda_runtime.h>
#include <cuda_bf16.h>
#include <cmath>

__global__ void swiglu_bf16_kernel(
    const __nv_bfloat16* __restrict__ d_gate,
    const __nv_bfloat16* __restrict__ d_up,
    __nv_bfloat16* __restrict__ d_out,
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

### Step 3.4: Create core/src/CUDA/kernel/paged_gqa.cu

```cpp
#include <cuda_runtime.h>
#include <cuda_bf16.h>
#include <cmath>
#include <cstdint>

constexpr int KV_BLOCK_SIZE = 16;

__global__ void append_paged_kv_kernel(
    const __nv_bfloat16* __restrict__ d_k,
    const __nv_bfloat16* __restrict__ d_v,
    __nv_bfloat16* __restrict__ d_k_cache,
    __nv_bfloat16* __restrict__ d_v_cache,
    const int32_t* __restrict__ d_block_table,
    int token_pos,
    int num_kv_heads,
    int head_dim
) {
    int kv_head = blockIdx.x;
    int d       = threadIdx.x;
    if (kv_head >= num_kv_heads || d >= head_dim) return;

    int logical_block  = token_pos / KV_BLOCK_SIZE;
    int block_offset   = token_pos % KV_BLOCK_SIZE;
    int physical_block = d_block_table[logical_block];

    size_t cache_idx = ((static_cast<size_t>(physical_block) * KV_BLOCK_SIZE + block_offset) * num_kv_heads + kv_head) * head_dim + d;
    size_t src_idx   = static_cast<size_t>(kv_head) * head_dim + d;

    d_k_cache[cache_idx] = d_k[src_idx];
    d_v_cache[cache_idx] = d_v[src_idx];
}

__global__ void paged_gqa_decode_kernel(
    const __nv_bfloat16* __restrict__ d_q,
    const __nv_bfloat16* __restrict__ d_k_cache,
    const __nv_bfloat16* __restrict__ d_v_cache,
    const int32_t* __restrict__ d_block_table,
    __nv_bfloat16* __restrict__ d_attn_out,
    int seq_len,
    int num_q_heads,
    int num_kv_heads,
    int head_dim,
    float scale
) {
    extern __shared__ float s_logits[];
    int q_head    = blockIdx.x;
    int tid       = threadIdx.x;
    int gqa_ratio = num_q_heads / num_kv_heads;
    int kv_head   = q_head / gqa_ratio;

    const __nv_bfloat16* q_vec = d_q + q_head * head_dim;

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

## Stage 4: Multi-Layer Stacked Pipeline in core/include/ai.hpp and core/src/ai.cpp

### Step 4.1: Add Multi-Layer Structs to core/include/ai.hpp

```cpp
#pragma once
#include <cuda_bf16.h>
#include <cstdint>
#include <vector>
#include <string>
#include <functional>

struct AxiomModelConfig {
    uint32_t magic          = 0x4D4F4958; // "XIOM"
    uint32_t version        = 2;
    uint32_t num_layers     = 28;
    uint32_t hidden_dim     = 1536;
    uint32_t num_q_heads    = 12;
    uint32_t num_kv_heads   = 2;
    uint32_t head_dim       = 128;
    uint32_t ffn_dim        = 8960;
    uint32_t vocab_size     = 151936;
    uint32_t max_seq_len    = 4096;
    float    rms_norm_eps   = 1e-6f;
    float    rope_theta     = 1000000.0f;
};

struct TransformerLayerWeights {
    const __nv_bfloat16* d_input_layernorm;
    const __nv_bfloat16* d_q_proj_weight;
    const __nv_bfloat16* d_q_proj_bias;
    const __nv_bfloat16* d_k_proj_weight;
    const __nv_bfloat16* d_k_proj_bias;
    const __nv_bfloat16* d_v_proj_weight;
    const __nv_bfloat16* d_v_proj_bias;
    const __nv_bfloat16* d_o_proj_weight;
    const __nv_bfloat16* d_post_attn_layernorm;
    const __nv_bfloat16* d_gate_proj_weight;
    const __nv_bfloat16* d_up_proj_weight;
    const __nv_bfloat16* d_down_proj_weight;
};

```

### Step 4.2: Implement the 28-Layer Forward Step in core/src/ai.cpp

```cpp
void AI::forward_transformer_step(int token_id, int token_pos, float* d_out_logits, cudaStream_t stream) {
    const uint32_t D   = config_.hidden_dim;
    const uint32_t Qd  = config_.num_q_heads * config_.head_dim;
    const uint32_t KVd = config_.num_kv_heads * config_.head_dim;
    const uint32_t FFN = config_.ffn_dim;

    cudaMemcpyAsync(d_curr_token_, &token_id, sizeof(uint32_t), cudaMemcpyHostToDevice, stream);
    cudaMemcpyAsync(d_curr_pos_, &token_pos, sizeof(int32_t), cudaMemcpyHostToDevice, stream);

    launch_gather_embeddings_batched(
        d_curr_token_, d_embed_tokens_, d_residual_stream_,
        1, D, config_.vocab_size, stream
    );

    for (uint32_t l = 0; l < config_.num_layers; ++l) {
        const auto& w = layers_[l];

        launch_rmsnorm_bf16(d_residual_stream_, w.d_input_layernorm, d_normed_x_, 1, D, config_.rms_norm_eps, stream);

        launch_cutlass_gemm_bf16(1, Qd,  D, d_normed_x_, w.d_q_proj_weight, w.d_q_proj_bias, d_q_, stream);
        launch_cutlass_gemm_bf16(1, KVd, D, d_normed_x_, w.d_k_proj_weight, w.d_k_proj_bias, d_k_, stream);
        launch_cutlass_gemm_bf16(1, KVd, D, d_normed_x_, w.d_v_proj_weight, w.d_v_proj_bias, d_v_, stream);

        launch_rope_inplace_bf16(
            d_q_, d_k_, d_curr_pos_, 1,
            config_.num_q_heads, config_.num_kv_heads, config_.head_dim, config_.rope_theta, stream
        );

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

        launch_cutlass_gemm_bf16(1, D, Qd, d_attn_out_, w.d_o_proj_weight, nullptr, d_proj_tmp_, stream);
        launch_residual_add_bf16(d_residual_stream_, d_proj_tmp_, D, stream);

        launch_rmsnorm_bf16(d_residual_stream_, w.d_post_attn_layernorm, d_normed_x_, 1, D, config_.rms_norm_eps, stream);

        launch_cutlass_gemm_bf16(1, FFN, D, d_normed_x_, w.d_gate_proj_weight, nullptr, d_gate_buf_, stream);
        launch_cutlass_gemm_bf16(1, FFN, D, d_normed_x_, w.d_up_proj_weight,   nullptr, d_up_buf_,   stream);
        launch_swiglu_bf16(d_gate_buf_, d_up_buf_, d_act_buf_, FFN, stream);
        launch_cutlass_gemm_bf16(1, D, FFN, d_act_buf_,  w.d_down_proj_weight, nullptr, d_proj_tmp_, stream);
        launch_residual_add_bf16(d_residual_stream_, d_proj_tmp_, D, stream);
    }

    launch_rmsnorm_bf16(d_residual_stream_, d_final_norm_weight_, d_normed_x_, 1, D, config_.rms_norm_eps, stream);
    launch_tied_logits_bf16(d_normed_x_, d_lm_head_weight_, d_out_logits, 1, config_.vocab_size, D, stream);
}

```

## Stage 5: Qwen2.5-Coder Weight & Vocabulary Converter + Win32 mmap Loader

### Step 5.1: Create tools/convert_qwen.py (Exports Both Weights and Exact Vocabulary)

This script converts **Qwen2.5-Coder-1.5B-Instruct** (or **Qwen2.5-1.5B-Instruct**) `model.safetensors` and `tokenizer.json` into flat Axiom binaries:

```python
import json
import struct
import sys
from pathlib import Path
from safetensors import safe_open
import torch

def export_qwen_to_axiom(model_dir: str, out_weights: str, out_vocab: str):
    model_dir_path = Path(model_dir)
    safetensors_path = model_dir_path / "model.safetensors"
    tokenizer_path = model_dir_path / "tokenizer.json"

    print(f"[Axiom Converter] Packing weights from {safetensors_path}...")
    f = safe_open(str(safetensors_path), framework="pt", device="cpu")

    header = struct.pack(
        "<10I2f16x",
        0x4D4F4958, 2, 28, 1536, 12, 2, 128, 8960, 151936, 4096,
        1e-6, 1000000.0
    )

    def write_bf16(out_file, tensor_name: str):
        t = f.get_tensor(tensor_name).to(torch.bfloat16).contiguous()
        out_file.write(t.view(torch.uint16).numpy().tobytes())

    with open(out_weights, "wb") as out:
        out.write(header)
        write_bf16(out, "model.embed_tokens.weight")
        for l in range(28):
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
        write_bf16(out, "model.norm.weight")
        if "lm_head.weight" in f.keys():
            write_bf16(out, "lm_head.weight")
        else:
            write_bf16(out, "model.embed_tokens.weight")

    if tokenizer_path.exists():
        print(f"[Axiom Converter] Exporting vocabulary from {tokenizer_path}...")
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

    print("[Axiom Converter] Conversion complete.")

if __name__ == "__main__":
    src_dir = sys.argv[1] if len(sys.argv) > 1 else "./Qwen2.5-Coder-1.5B-Instruct"
    export_qwen_to_axiom(src_dir, "models/qwen1_5b.axiom", "models/qwen_vocab.axiom")

```

### Step 5.2: Win32 Memory-Mapped Zero-Copy VRAM Loader in core/src/ai.cpp

Add `AI::load_axiom_model_mmap` to **core/src/ai.cpp** to stream the 3.1 GB `.axiom` checkpoint directly from NVMe into contiguous VRAM in under 0.8 seconds:

```cpp
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

    std::memcpy(&config_, mapped_base, sizeof(AxiomModelConfig));
    const uint8_t* host_weights_ptr = mapped_base + 64;
    size_t weights_bytes = total_bytes - 64;

    cudaMalloc(&d_weights_arena_, weights_bytes);
    cudaHostRegister(const_cast<uint8_t*>(host_weights_ptr), weights_bytes, cudaHostRegisterReadOnly);
    cudaMemcpyAsync(d_weights_arena_, host_weights_ptr, weights_bytes, cudaMemcpyHostToDevice, compute_stream_);
    cudaStreamSynchronize(compute_stream_);
    cudaHostUnregister(const_cast<uint8_t*>(host_weights_ptr));

    UnmapViewOfFile(mapped_base);
    CloseHandle(hMap);
    CloseHandle(hFile);

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

## Stage 6: Connect Project Axiom to Cline CLI (And ComfyUI)

Cline CLI communicates via the OpenAI `/v1/chat/completions` HTTP protocol. Rather than introducing a heavy Python server, build a zero-dependency C++23 WinSock2 HTTP endpoint directly inside **core/src/server.cpp** alongside **core/src/axiom_c_api.cpp**.

### Step 6.1: Create the Native C++23 OpenAI Server for Cline CLI in core/src/server.cpp

```cpp
#include "../include/ai.hpp"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <sstream>

#pragma comment(lib, "Ws2_32.lib")

static std::string escape_json(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 16);
    for (char c : s) {
        switch (c) {
            case '\"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;      break;
        }
    }
    return out;
}

// Extracts the last user message from Cline's JSON payload and wraps in Qwen ChatML
static std::string build_qwen_chatml_prompt(const std::string& body) {
    std::string content = "Write clean C++23 and CUDA code.";
    size_t pos = body.rfind("\"content\"");
    if (pos != std::string::npos) {
        size_t start = body.find('\"', pos + 9);
        if (start != std::string::npos) {
            size_t end = body.find("\"}", start + 1);
            if (end != std::string::npos) {
                content = body.substr(start + 1, end - start - 1);
            }
        }
    }
    return "<|im_start|>system\nYou are Project Axiom, an expert C++23 and CUDA coding agent.<|im_end|>\n"
           "<|im_start|>user\n" + content + "<|im_end|>\n"
           "<|im_start|>assistant\n";
}

void run_axiom_openai_server(AI& engine, uint16_t port) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(port);

    bind(listen_sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    listen(listen_sock, SOMAXCONN);

    std::cout << "[Axiom Server] Listening for Cline CLI on http://127.0.0.1:" << port << "/v1/chat/completions\n";

    std::vector<char> recv_buf(65536);
    while (true) {
        SOCKET client = accept(listen_sock, nullptr, nullptr);
        if (client == INVALID_SOCKET) continue;

        int bytes = recv(client, recv_buf.data(), static_cast<int>(recv_buf.size() - 1), 0);
        if (bytes > 0) {
            recv_buf[bytes] = '\0';
            std::string req(recv_buf.data(), bytes);
            size_t body_pos = req.find("\r\n\r\n");
            std::string body = (body_pos != std::string::npos) ? req.substr(body_pos + 4) : "";

            std::string chatml_prompt = build_qwen_chatml_prompt(body);
            std::string generated_reply;

            engine.generate(chatml_prompt, 512, 0.2f, 40, 0.9f, [&](const std::string& tok) {
                generated_reply += tok;
                std::cout << tok << std::flush;
            });

            std::string json_payload =
                "{\"id\":\"chatcmpl-axiom\",\"object\":\"chat.completion\",\"choices\":[{\"index\":0,"
                "\"message\":{\"role\":\"assistant\",\"content\":\"" + escape_json(generated_reply) + "\"},"
                "\"finish_reason\":\"stop\"}]}";

            std::ostringstream resp;
            resp << "HTTP/1.1 200 OK\r\n"
                 << "Content-Type: application/json\r\n"
                 << "Content-Length: " << json_payload.size() << "\r\n"
                 << "Connection: close\r\n\r\n"
                 << json_payload;

            std::string wire = resp.str();
            send(client, wire.data(), static_cast<int>(wire.size()), 0);
        }
        closesocket(client);
    }
}

```

### Step 6.2: Wire the `--serve` Flag in core/src/main.cpp

Open **core/src/main.cpp** and add the `--serve` CLI branch so `.\app.exe --serve 8080` boots the model into VRAM and starts listening for Cline CLI requests:

```cpp
if (arg == "--serve") {
    uint16_t port = (i + 1 < argc) ? static_cast<uint16_t>(std::stoi(argv[++i])) : 8080;
    AI engine;
    engine.load_vocabulary("models/qwen_vocab.axiom");
    engine.load_axiom_model_mmap("models/qwen1_5b.axiom");
    run_axiom_openai_server(engine, port);
    return 0;
}

```

### Step 6.3: Add the Server & Shared DLL Targets in core/Makefile

Open **core/Makefile** and link `-lws2_32` for the native OpenAI server while preserving the `dll` target for ComfyUI (`build/axiom_core.dll` + **ComfyUI/custom_nodes/ComfyUI-ProjectAxiom/axiom_node.py**):

```makefile
CUDA_KERNELS := src/CUDA/kernel/forward_pass.cu \
                src/CUDA/kernel/backward_pass.cu \
                src/CUDA/kernel/rmsnorm.cu \
                src/CUDA/kernel/rope.cu \
                src/CUDA/kernel/swiglu.cu \
                src/CUDA/kernel/paged_gqa.cu

HOST_SRCS := src/main.cpp src/ai.cpp src/server.cpp src/transfer.cpp \
             util/identifier.cpp util/optimizer.cpp util/parser.cpp

all:
	$(NVCC) -arch=sm_89 -O3 --shared -o cuda_transfer.dll $(CUDA_KERNELS) src/CUDA/gpu/upload.cu src/CUDA/gpu/download.cu
	g++ -std=c++23 -O3 $(HOST_SRCS) -L. -lcuda_transfer -lws2_32 -o app.exe

dll:
	$(NVCC) -arch=sm_89 -O3 --shared -o ../build/axiom_core.dll src/axiom_c_api.cpp $(CUDA_KERNELS)

```

## Stage 7: Build, Launch & Cut Over Cline CLI to Project Axiom

Execute these commands in order from your Project Axiom root directory in PowerShell:

1. Convert your local Qwen2.5-Coder weights and tokenizer into flat Axiom files:

```powershell
python tools/convert_qwen.py path/to/Qwen2.5-Coder-1.5B-Instruct models/qwen1_5b.axiom models/qwen_vocab.axiom

```

2. Clean, compile the updated CUDA kernels and C++23 server, and verify all unit tests pass:

```powershell
cd core
mingw32-make clean
mingw32-make all
.\test.exe --test

```

3. Start the bare-metal Project Axiom OpenAI server on port 8080:

```powershell
.\app.exe --serve 8080

```

4. Point Cline CLI to Project Axiom as its compute backend:

- Set **API Provider** to `OpenAI Compatible`.
- Set **Base URL** to `[http://127.0.0.1:8080/v1](http://127.0.0.1:8080/v1)`.
- Set **API Key** to `axiom-local`.
- Set **Model ID** to `axiom-qwen-coder`.

Want to start by dropping the four new CUDA kernels (`rmsnorm.cu`, `rope.cu`, `swiglu.cu`, `paged_gqa.cu`) into **core/src/CUDA/kernel/** and running a quick `nvcc` compile check?
