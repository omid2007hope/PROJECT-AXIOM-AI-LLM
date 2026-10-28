# Project Axiom: Master Engineering Roadmap (Version 2.0)

**Document Path:** `document/plan/plan.md`  
**Target Hardware:** NVIDIA Ada Lovelace (`sm_89` / RTX 4080 Laptop GPU) & Native C++23 / CUDA 13.3  
**Status:** Active Engineering Directive — Horizon Execution Sequence  
**Single Source of Truth:** [`document/plan/UnifiedMasterEngineeringPlan.md`](file:///c:/Users/Omid/X/Code/PROJECT-AXIOM/document/plan/UnifiedMasterEngineeringPlan.md)  
**Historical Archive:** [`document/plan/archive_plan_pre_19_oct.md`](file:///c:/Users/Omid/X/Code/PROJECT-AXIOM/document/plan/archive_plan_pre_19_oct.md) & [`document/plan/Project_Axiom_Plan.md`](file:///c:/Users/Omid/X/Code/PROJECT-AXIOM/document/plan/Project_Axiom_Plan.md)

---

## 1. Executive Baseline & Scope Directive

All foundational Phase 0 systems plumbing—including zero-copy pinned DMA transfers, multi-stream double buffering, dense early-fusion codebook ($V = 65,536$), CUTLASS v4.8.0 Tensor Core execution, linear EVT epilogues, branchless VRAM embedding gather/scatter kernels, and atomic AdamW state persistence—is **100% completed, tested, and verified (314/314 tests passing green)**.

Furthermore, **Horizon 1 (Multi-Token Autoregressive Generation, Sampling & Corpus Hygiene)** is fully implemented and verified with live CLI terminal streaming and frequency-filtered vocabulary ingestion.

This document defines the **active development roadmap moving forward**. The engineering focus is strictly on surgical VRAM sync eradication, scaling to batched multi-head attention, and zero-overhead graph execution.

---

## 2. Strategic Execution Horizons

```
                            PROJECT AXIOM — FUTURE EXECUTION PHASES
  ┌────────────────────────────────────────────────────────────────────────────────────────┐
  │ [x] HORIZON 1: Multi-Token Autoregressive Generation & Sampling [COMPLETED & VERIFIED] │
  │ • Host generation loop: continuous autoregressive token feedback (AI::generate)        │
  │ • Stochastic sampling engine: Temperature scaling, Top-K, Top-P Nucleus, ArgMax        │
  │ • Corpus hygiene: Frequency filtering (count >= 3) and <UNK> out-of-bounds routing     │
  └───────────────────────────────────────────────────┬────────────────────────────────────┘
                                                      │
  ┌───────────────────────────────────────────────────▼────────────────────────────────────┐
  │ [ ] HORIZON 2: Surgical Sync Eradication & Production VRAM Management [IMMEDIATE NEXT] │
  │ • Eradicate dynamic cudaMalloc in upload.cu via stream memory pools / BFC allocator    │
  │ • Non-blocking loss telemetry in download.cu via pinned slot and cudaEventQuery        │
  │ • Lock-free SPSC ring buffer & background disk prefetch in optimizer.cpp               │
  └───────────────────────────────────────────────────┬────────────────────────────────────┘
                                                      │
  ┌───────────────────────────────────────────────────▼────────────────────────────────────┐
  │ [ ] HORIZON 3: Multi-Head Self-Attention (MHA / GQA) & Transformer Blocks [PENDING]    │
  │ • Linear Q, K, V projections on Tensor Cores via batched CUTLASS GEMMs                 │
  │ • Fused Scaled Dot-Product Attention (SDPA) with causal sequence masking               │
  │ • Residual skip connections & SwiGLU / MLP feed-forward blocks                         │
  └───────────────────────────────────────────────────┬────────────────────────────────────┘
                                                      │
  ┌───────────────────────────────────────────────────▼────────────────────────────────────┐
  │ [ ] HORIZON 4: Production VRAM & PagedAttention KV-Cache State Management [PENDING]    │
  │ • Partition Key/Value caches into fixed-size physical pages (16 tokens per block)      │
  │ • CPU-managed Block Table mapping logical token indices to physical VRAM blocks        │
  │ • Copy-On-Write (COW) prompt prefix caching for multi-turn conversations               │
  └───────────────────────────────────────────────────┬────────────────────────────────────┘
                                                      │
  ┌───────────────────────────────────────────────────▼────────────────────────────────────┐
  │ [ ] HORIZON 5: Zero-Overhead Hardware Saturation via CUDA Graphs & ATen [PENDING]      │
  │ • Static Directed Acyclic Graph (DAG) capture with cudaStreamBeginCapture              │
  │ • Single-driver launch via cudaGraphLaunch (<0.05 ms host scheduling latency)          │
  │ • Decouple Tensor from TensorImpl; construct central C++ DispatchKeySet dispatcher     │
  └───────────────────────────────────────────────────┬────────────────────────────────────┘
                                                      │
  ┌───────────────────────────────────────────────────▼────────────────────────────────────┐
  │ [ ] HORIZON 6: Sparse Mixture of Experts (MoE) & Multimodal Output Codecs [PENDING]    │
  │ • Partition dense FFN blocks into N = 8 specialized expert weight stacks in VRAM       │
  │ • Lightweight Top-2 Router Network dispatching tokens with zero inactive FLOPs         │
  │ • Generative Output Codecs: Discrete visual/audio token unrolling to BMP/MP4 and WAV   │
  └────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 3. Horizon Specifications & Engineering Tasks

### Horizon 1: Multi-Token Autoregressive Generation & Sampling Loop [COMPLETED & VERIFIED]

- [x] **1.1. Host Generation Loop (`core/src/ai.cpp`, `core/src/main.cpp`):**
  - Implemented `AI::generate(prompt, parser, input, max_new_tokens, config, token_callback)`.
  - Tokenizes input prompt, fills 128-token FIFO sliding window, computes tied logits ($Y \cdot E^T$).
  - Streams sampled token strings to terminal in real time via lambda callback.
  - Slices context window forward on each step until `vocab::EOS` or limit.
- [x] **1.2. Stochastic Sampling Engine (`core/src/CUDA/kernel/forward_pass.cu`, `core/src/ai.cpp`):**
  - Temperature scaling: $\hat{z}_i = z_i / T$ with $T \in (0.0, 2.0]$.
  - Top-K logit filtering and Top-P (nucleus) cumulative distribution truncation.
  - Greedy ArgMax fallback on GPU (`softmax_argmax_kernel`) when $T \le 0.001$.
  - Bit-identical deterministic reproducibility verified with fixed seeds.
- [x] **1.3. Vocabulary Bounding & Corpus Hygiene (`core/util/parser.cpp`):**
  - Implemented frequency filtering ($\text{count} \ge 3$) during bulk corpus ingestion.
  - Prevents typos and transient tokens from polluting the 64,508 `main_dictionary` partition.
  - Safely routes sub-threshold and out-of-bounds tokens to `vocab::UNK` (ID `1`).
- [x] **Gate 1 Verified:** Measured generation throughput $>50\text{ tokens/sec}$ on RTX 4080 (up to 2,625 tok/s on Tensor Cores).

---

### Horizon 2: Surgical Sync Eradication & Production VRAM Management [IMMEDIATE NEXT TARGET]

- [ ] **2.1. Eradicate Dynamic `cudaMalloc` in Transfer Loops (`core/src/CUDA/gpu/upload.cu`):**
  - Remove lines 20–28 dynamic allocation guards in `upload_to_gpu`.
  - Transition temporary buffers to stream-ordered memory pools (`cudaMallocAsync` / `cudaFreeAsync`).
  - Implement a userspace **Best-Fit with Coalescing (BFC) Caching Allocator** over a pre-allocated multi-gigabyte VRAM arena.
- [ ] **2.2. Non-Blocking Loss Telemetry (`core/src/CUDA/gpu/download.cu`):**
  - Eliminate device-wide `cudaDeviceSynchronize()` barrier inside `download_loss`.
  - Asynchronously copy scalar loss into a 4-byte pinned host slot via `cudaMemcpyAsync`.
  - Poll stream events non-blockingly via `cudaEventQuery`, preserving double-buffered compute.
- [ ] **2.3. Lock-Free SPSC Ring Buffer & Async Disk Prefetch (`core/util/optimizer.cpp`):**
  - Replace atomic polling spin-locks on `Portion::buffer_in_use` and `Portion::context_in_use`.
  - Implement a lock-free Single-Producer Single-Consumer (SPSC) ring buffer.
  - Run background producer thread to prefetch the next 100,000-token chunk from disk to RAM ahead of GPU consumption.
- [ ] **Gate 2 Verification:** Exactly 0 calls to `cudaMalloc` or `cudaDeviceSynchronize` during active loops; zero GPU starvation bubbles verified via Nsight Systems trace.

---

### Horizon 3: Multi-Head Self-Attention (MHA / GQA) & Transformer Blocks [PENDING]

- [ ] **3.1. Batched Tensor Core Projections & Backward GEMM:**
  - Project hidden states into Query, Key, and Value representations:
    $$Q = X W_Q, \quad K = X W_K, \quad V = X W_V$$
  - Upgrade backward pass (`backward_pass.cu`) from memory-bound Rank-1 outer products to batched CUTLASS GEMMs.
- [ ] **3.2. Fused Scaled Dot-Product Attention (SDPA) & Causal Masking:**
  - Implement fused FlashAttention-style CUDA kernel:
    $$\text{Attention}(Q, K, V) = \text{Softmax}\left(\frac{Q K^T}{\sqrt{d_{\text{head}}}} + M\right) V$$
  - Apply causal mask $M_{i, j} = 0$ for $j \le i$, and $-\infty$ for $j > i$ to prevent future token leakage.
- [ ] **3.3. Pre-Norm Transformer Layer Composition:**
  - Residual connection: $X_{\text{mid}} = X + \text{Attention}(\text{RMSNorm}(X))$.
  - SwiGLU Feed-Forward Network: $\text{FFN}(x) = (\text{Swish}(x W_{\text{gate}}) \odot x W_{\text{up}}) W_{\text{down}}$.
  - Layer output: $X_{\text{out}} = X_{\text{mid}} + \text{FFN}(\text{RMSNorm}(X_{\text{mid}}))$.
- [ ] **Gate 3 Verification:** Monotonic loss reduction on causal sequence modeling; Tensor Core backward GEMMs active.

---

### Horizon 4: Production VRAM & PagedAttention State Management [PENDING]

- [ ] **4.1. Fixed-Size Physical KV Blocks:**
  - Partition Key and Value cache memory into fixed-size physical pages (16 tokens per block).
  - Eradicate internal and external VRAM fragmentation during variable-length autoregressive decoding.
- [ ] **4.2. CPU Block Table & Copy-On-Write (COW) Prefix Caching:**
  - Maintain CPU-side Block Table mapping logical token indices $(0 \dots T-1)$ to physical VRAM blocks.
  - Implement Copy-On-Write semantics so shared prompts and multi-turn conversations reference identical physical blocks in VRAM.
- [ ] **Gate 4 Verification:** 100% block reuse verified across multi-turn prompts; zero allocation failures under stress testing.

---

### Horizon 5: Zero-Overhead Hardware Saturation via CUDA Graphs & Modular ATen Dispatcher [PENDING]

- [ ] **5.1. Static DAG Graph Capture:**
  - Capture static compute topology into a single hardware Directed Acyclic Graph via `cudaStreamBeginCapture` and `cudaStreamEndCapture`.
  - Dispatch iterations via `cudaGraphLaunch`, reducing host CPU launch latency to $<0.05\text{ ms}$.
- [ ] **5.2. Modular C++ ATen Dispatcher & Autograd Tape:**
  - Decouple lightweight `Tensor` handles from reference-counted `TensorImpl` (shape, stride, device metadata).
  - Central `DispatchKeySet` routing across CPU and CUDA backends with automated reverse-mode autograd tape.
- [ ] **Gate 5 Verification:** Host launch latency $<0.05\text{ ms}$; autograd tape gradients match manual analytical gradients.

---

### Horizon 6: Sparse Mixture of Experts (MoE) Architecture & Output Codecs [PENDING]

- [ ] **6.1. Sparse Mixture of Experts (MoE) Layers:**
  - Decompose dense FFN layers into $N = 8$ discrete expert weight stacks in VRAM.
  - Lightweight gating router: $G(x) = \text{TopK}(\text{Softmax}(x W_g), k=2)$. Inactive experts consume zero arithmetic FLOPs.
- [ ] **6.2. Multimodal Generative Output Codecs:**
  - Visual Codec: Decode discrete visual tokens (`[64,512..64,767]`) to RGB pixel buffers and serialize to BMP/MP4 containers.
  - Acoustic Codec: Decode discrete acoustic tokens (`[64,768..65,023]`) to linear PCM waveforms with valid RIFF `.wav` headers.
- [ ] **Gate 6 Verification:** Top-2 expert load balancing verified; valid `.bmp` and `.wav` reconstructed from token streams.

---

## 4. Verification Gates & Acceptance Protocol Summary

| Gate | Milestone Name | Target Metric | Verification Method | Status |
| :---: | :--- | :--- | :--- | :---: |
| **Gate 0** | **Foundations Locked** | 314/314 unit & stress tests passing (100% Green) | `mingw32-make test` (`test.exe --test`) | **PASSED** |
| **Gate 1** | **Multi-Token Streaming & Sampling** | $>50\text{ tokens/sec}$; $T > 0$ diverse text without `<UNK>` saturation | Interactive prompt CLI in `main.cpp` & `test_inference.cpp` | **PASSED** |
| **Gate 2** | **Zero Runtime Allocations & Syncs** | Exactly 0 `cudaMalloc` or `cudaDeviceSynchronize` in loops | Nsight Systems trace & code audit of `upload.cu`/`download.cu` | **NEXT** |
| **Gate 3** | **Attention Layer Convergence** | Monotonic loss reduction on causal language modeling | Multi-step loss validation in `test_ai.cpp` | **PENDING** |
| **Gate 4** | **Zero KV Fragmentation** | 100% block reuse via COW prefix sharing | PagedAttention unit tests | **PENDING** |
| **Gate 5** | **Sub-Microsecond Graph Dispatch** | $<0.05\text{ ms}$ launch latency; autograd tape parity | `cudaEventElapsedTime` profiling | **PENDING** |
| **Gate 6** | **MoE Routing & Output Codecs** | Top-2 expert routing; valid `.bmp` and `.wav` synthesis | Router tests & `test_decom.cpp` | **PENDING** |
