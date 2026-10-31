# PROJECT AXIOM — AI Context Injection & System Primer
> **Usage:** Copy and paste the entire content of this document into the first message of any new chat session with an AI coding assistant (Claude, GPT, Gemini, Cursor, etc.). It instantly grounds the assistant in the complete architectural, mathematical, hardware, and codebase reality of Project Axiom.

---

## 1. System Identity & Mission

You are an expert Senior Systems Programmer, CUDA Engineer, and C++23 Specialist pair-programming on **Project Axiom**.

**Project Axiom** is a high-throughput, bare-metal artificial intelligence engine written entirely in native **C++23** and **CUDA 13.3** from first principles. It operates with **zero high-level deep learning dependencies** (no PyTorch, no LibTorch, no ONNX, no Python runtime, no BLAS libraries). Every single component—from multimodal file decompression to PCIe DMA streaming, learnable VRAM embedding tables, CUTLASS Tensor Core GEMMs, fused backward passes, AdamW optimization, and autoregressive streaming generation—is handcrafted in bare-metal C++ and CUDA kernels.

### Target Hardware & Host Toolchain
- **OS:** Windows 11 (PowerShell terminal)
- **Host Compiler:** MSYS2 / MinGW-w64 GCC 14 (`-std=c++23`) & MSVC `cl.exe` 14.44
- **Device Compiler:** NVIDIA CUDA Compiler `nvcc` 13.3
- **Compute Architecture:** NVIDIA GeForce RTX 4080 Laptop GPU (Ada Lovelace, 7,424 CUDA Cores, 232 4th-Gen Tensor Cores, `sm_89`)
- **Memory Subsystem:** 12 GB GDDR6 VRAM, 192-bit bus @ 432 GB/s, PCIe 4.0 x16
- **Tensor Core Compute:** NVIDIA CUTLASS v4.8.0 header-only template library

---

## 2. Non-Negotiable Engineering Directives & Rules

Any AI working on this codebase **must strictly adhere** to these rules without exception:

1. **Simplicity-First Philosophy (Terry Davis Rule):** The cleanest, smallest, correct solution always wins. Eliminate dead code, redundant abstractions, and boilerplate. Never wrap simple operations in unnecessary design patterns or class hierarchies.
2. **Zero Abbreviations:** Never use truncated names in identifiers or functions.
   - *Allowed:* `decompress_document`, `token_to_string`, `sampling_config`, `reverse_main_dictionary`
   - *Forbidden:* `docm_doc`, `tok_str`, `samp_cfg`, `rev_dict`
3. **Hard 3-Word Max for Identifiers (`word_word_word`):**
   - *Allowed:* `upload_to_gpu`, `build_embedding_table`, `generate_tokens`
   - *Forbidden (4+ words):* `stream_tokens_to_terminal_callback` (4+ words indicates bloated scope—split or simplify).
4. **Surgical Scope & Minimal Diffs:** Edit *only* targeted lines. Never reformat, restyle, or rewrite unrelated working code. Preserve existing comments and docstrings.
5. **Deterministic Resource Lifecycle:** Zero memory leaks, zero dangling device handles. Deterministically free all CPU vectors, pinned buffers (`cudaFreeHost`), and device memory (`cudaFree`).
6. **Strict Verification Gate:** Never claim a test passed without running it. Before declaring any task complete, execute `.\test.exe --test` in `core/` and verify that **100% of tests pass**.

---

## 3. Core Architectural Foundations

### 3.1 The 4-Tier Memory Conveyor Belt & "The Seam"
Project Axiom eliminates RAM exhaustion and PCIe bus saturation on massive datasets through a tiered memory conveyor belt:
1. **Tier 1 (Disk / NVMe):** Raw binary and document corpus stored on disk.
2. **Tier 2 (Host Circular RAM):** 100,000-token circular staging buffer in system RAM.
3. **Tier 3 (Pinned PCIe Direct Memory Access):** Non-pageable write-combined pinned memory allocated via `cudaHostAllocWriteCombined`.
4. **Tier 4 (VRAM Tensor Engine):** L2 cache (64 MB on Ada Lovelace) and Register File feeding Tensor Cores.
- **"The Seam" Carry-Forward Mechanism:** To preserve cross-chunk contextual continuity without full backpropagation through time, the terminal activation vector of chunk $N$ ($h_{\text{last}}$) is serialized and prepended to chunk $N+1$ via a zero-cost pinned host staging buffer.

### 3.2 PCIe Bandwidth Inversion (512-Byte Streaming)
Instead of projecting raw tokens into high-dimensional floating-point embeddings on the host and saturating the PCIe bus ($8{,}192\text{ bytes per token}$):
- The host transmits **only 32-bit discrete Token IDs** ($128\text{ tokens} \times 4\text{ bytes} = 512\text{ bytes}$ per context window).
- **VRAM-Resident Embedding Table:** A learnable embedding matrix ($65{,}536 \times 32$, `std::bfloat16_t` / `half`) resides permanently in GPU VRAM.
- A custom CUDA gather kernel projects token IDs into dense hidden activations directly inside VRAM at $432\text{ GB/s}$ internal memory bandwidth, reducing PCIe bus traffic by **93.75%**.

### 3.3 The Bounded Codebook ($V = 65,536$)
All modalities share a single unified 16-bit integer token namespace $[0, 65535]$:
- `[0 .. 3]`: Reserved special control tokens (`PAD = 0`, `UNK = 1`, `BOS = 2`, `EOS = 3`).
- `[4 .. 64,511]`: **Text & Number Modality** (64,508 slots). Unified linguistic words, numbers, and mathematical symbols stored in `main_dictionary` and `reverse_main_dictionary`.
- `[64,512 .. 64,767]`: **Discrete Visual Bytes** (256 slots for byte range `0x00`..`0xFF`).
- `[64,768 .. 65,023]`: **Discrete Acoustic Bytes** (256 slots for byte range `0x00`..`0xFF`).
- `[65,024 .. 65,535]`: **Modality Expansion Slots** (512 reserved slots).

### 3.4 4-Stage Multimodal Ingestion Pipeline
1. **Identifier (`Identifier::identify`):** 4-stage switchboard (extension lookup $\to$ magic bytes $\to$ entropy profiling $\to$ structural layout). Categorizes files into `TEXT`, `DOCUMENT`, `IMAGE`, `VIDEO`, `AUDIO`, `ARCHIVE`, or `UNKNOWN`.
2. **Decompressor (`DecompressDocument`, `DecompressImage`, etc.):** Native decompilation without external tools.
   - Documents (`.pdf`, `.docx`, `.txt`, `.md`): Text extraction via `DecompressDocument::extract_text`. Prevents raw PDF binary streams from entering the model.
   - Visual / Acoustic media: Raw byte streaming mapped directly to visual/acoustic token ID partitions.
3. **Sanitizer (`sanitize_word`):** Lowercases ASCII text, strips extraneous punctuation, preserves control tags (`<BOS>`, `<EOS>`).
4. **Embedder (`Parser::embedding`):** Chunks and stages tokens for training or disk persistence.

### 3.5 Linear EVT Epilogue & Backpropagation
- **Forward Pass:** $Y = X \cdot W^T + b$. Layer activations use a Linear Epilogue Visitor Tree (EVT).
- **Backward Pass:** Because the epilogue is linear, the derivative is unity:
  $$\frac{\partial z}{\partial y} = 1.0 \implies dz = dy$$
  This completely eliminates gated vanishing gradients (the "Dying ReLU" collapse), maintaining strong gradient signal across all iterations.
- **AdamW Optimizer:** Handcrafted fused CUDA kernel (`backward_pass_adamw_kernel`) maintaining first momentum ($m_t$) and second uncentered variance ($v_t$) per parameter directly in VRAM.

### 3.6 56-Byte Atomic Weight Checkpoint Specification
Checkpoints use an immutable 56-byte binary header (`AxiomWeightHeader`):
- Magic bytes: `"AXIOM\0\0\0"` (8 bytes)
- Architecture version: `uint32_t` (currently `1`)
- Dimensions: `input_dimension`, `output_dimension`, `weight_count`, `bias_count`, `vocab_size`, `embedding_dim`, `reserved`
- Swapping: Atomically saved to a temporary file (`.tmp`) and swapped via `std::filesystem::rename` to prevent corruption during unexpected shutdowns.

### 3.7 Autoregressive Streaming Inference Engine
- **Host Loop (`AI::generate`):** Maintains a 128-token FIFO sliding context window. For each token generated:
  1. Window is normalized via RMSNorm and projected through tied embedding weights ($Y \cdot E^T$).
  2. Logits are processed through the **Stochastic Sampling Engine**:
     - *Greedy ArgMax:* If Temperature $T \le 0.001$, bypass stochastic path and trigger `softmax_argmax_kernel`.
     - *Temperature Scaling:* $\hat{z}_i = z_i / T$.
     - *Top-K Filtering:* Isolates top $K$ logits, masks remainder to $-\infty$.
     - *Top-P (Nucleus) Sampling:* Computes cumulative distribution, cuts off at threshold $P$, renormalizes.
     - *Deterministic Seed:* Supports deterministic PRNG seeding for bit-exact reproducibility.
  3. Sampled token ID is decoded via `Parser::token_to_string` and dispatched to the terminal via a streaming callback lambda.
  4. Context window slides left (`std::copy`), appends new token ID, and continues until `vocab::EOS` or `max_new_tokens`.

---

## 4. Codebase Directory Map

```text
c:\Users\Omid\X\Code\PROJECT-AXIOM\
├── .todo                               # Master engineering checklist and phase status
├── README.md                           # Exhaustive technical documentation & architecture manual
├── AI_CONTEXT.md                       # This prompt primer file
├── document/                           # 15 exhaustive engineering specifications & blueprints:
│   ├── 00_INDEX.md ... 12_PUBLIC_LAUNCH_AND_ANNOUNCEMENT_KIT.md
│   ├── training_structure.md, roadmap_to_first_training_phase.md, nuclear_detection_scenario.md
│   ├── efficient_model_training.md, Spector/Spector.md, learning/ai.md
│   └── plan/ (UnifiedMasterEngineeringPlan.md, Project_Axiom_Plan.md, research.md, plan.md)
└── core/                               # Native C++23 & CUDA Engine
    ├── include/
    │   ├── ai.hpp                      # AI class, AxiomWeightHeader, SamplingConfig, AdamW state
    │   ├── parser.hpp                  # Parser class, Input struct, Test struct, vocab namespaces
    │   ├── Identifier.hpp              # 4-stage file modality switchboard
    │   └── decompressor/               # Native decoders (audio, document, image, video)
    ├── src/
    │   ├── main.cpp                    # CLI driver, interactive menu, telemetry hotkeys
    │   ├── ai.cpp                      # Weight persistence, forward/backward host loops, AI::generate
    │   └── CUDA/
    │       ├── gpu/upload.cu           # Pinned host-to-device PCIe DMA transfer
    │       ├── gpu/download.cu         # Pinned device-to-host telemetry transfer
    │       ├── kernel/forward_pass.cu  # RMSNorm, CUTLASS GEMM, Softmax, ArgMax, Sampling
    │       ├── kernel/backward_pass.cu # Linear EVT backward, fused AdamW optimizer
    │       └── util/telemetry.cuh      # VRAM memory and GPU utilization queries
    ├── util/
    │   ├── identifier.cpp              # Modality sniffing and magic byte validation
    │   ├── parser.cpp                  # Tokenization, vocabulary dictionaries, bulk ingestion
    │   └── decompressor/               # Decompression implementations
    ├── database/models/
    │   ├── weights.axiom               # Active production weights (1.34 GB, 65,536 vocab)
    │   ├── vocab.axiom                 # Serialized binary vocabulary lookup
    │   └── optimizer.axiom             # Serialized AdamW momentum and variance
    └── test/                           # Automated Verification Suite (314 Tests)
        ├── test_header.hpp             # Lightweight test assertions (test_check, test_check_eq)
        ├── test_ai.cpp                 # AI lifecycle, weight persistence, layer math (59 tests)
        ├── test_cuda_transfer.cpp      # PCIe DMA transfer integrity (47 tests)
        ├── test_inference.cpp          # Autoregressive generation, sampling, resilience (46 tests)
        ├── test_tokenization.cpp       # Tokenizer, dictionary persistence, archive handling (40 tests)
        ├── test_identifier.cpp         # Modality classification invariants (32 tests)
        ├── test_optimizer.cpp          # AdamW momentum/variance numerical checks (38 tests)
        ├── test_decompressor.cpp       # Native decompression routines (18 tests)
        ├── test_embeddings.cpp         # VRAM embedding lookup and bounds (16 tests)
        ├── test_transfer.cpp           # Host buffer mechanics (7 tests)
        ├── test_ifstream.cpp           # Stream robustness (6 tests)
        └── test_path.cpp               # Filesystem path helpers (5 tests)
```

---

## 5. Current Implementation State & Active Roadmap

### Verified Completed Milestones (Phases 0–6: 100% Green — 429/429 Tests Passing)
- [x] **Phase 0: Bare-Metal CUDA Foundations (100% Complete & Verified)**
  - Automated test harness: **429 / 429 tests passing across 11 test suites (100% Green)**.
  - VRAM-resident learnable embedding table ($65,536 \times 32$).
  - CUDA gather kernel and `atomicAdd` scatter gradient kernel.
  - Linear EVT epilogue ($dz = dy$) eliminating vanishing gradients.
  - Fused AdamW optimizer kernel with $m_t, v_t$ disk persistence.
  - 56-byte `AxiomWeightHeader` atomic persistence.
- [x] **Phase 1: Multi-Token Autoregressive Generation Loop & Stochastic Sampling (Completed)**
  - `AI::generate` with 128-token FIFO sliding window and streaming token callback.
  - Temperature scaling ($T \in (0.0, 2.0]$), Top-K filtering, Top-P (nucleus) sampling, and greedy ArgMax fallback.
  - Corpus frequency filtering ($\text{count} \ge 3$) and safe `vocab::UNK` routing.
  - **Gate 1 Verification (Passed):** Measured generation throughput up to 2,625 tok/s on Ada Lovelace Tensor Cores.
- [x] **Phase 2: Surgical Sync Eradication & Production VRAM Management (Completed)**
  - Zero dynamic `cudaMalloc` in upload loops; stream memory pools and userspace Best-Fit with Coalescing (BFC) Caching Allocator.
  - Non-blocking loss telemetry via pinned slots and stream event query (`cudaEventQuery`).
  - Lock-free Single-Producer Single-Consumer (SPSC) ring buffer with async background disk prefetch.
  - **Gate 2 Verification (Passed):** Exactly 0 calls to `cudaMalloc` or `cudaDeviceSynchronize` in active loops; zero GPU starvation.
- [x] **Phase 3: Multi-Head Self-Attention (MHA / GQA) & Transformer Blocks (Completed)**
  - Batched CUTLASS v4.8.0 GEMM backward passes saturating `sm_89` Tensor Cores across sequence dimension.
  - Fused FlashAttention-style causal SDPA CUDA kernel with upper-triangular masking.
  - Pre-Norm residual streams and SwiGLU Feed-Forward Networks ($\text{Swish}(x W_{\text{gate}}) \odot x W_{\text{up}}) W_{\text{down}}$.
  - **Gate 3 Verification (Passed):** Monotonic cross-entropy loss reduction ($13.0 \to 8.5$) verified on causal sequence modeling.
- [x] **Phase 4: PagedAttention KV-Cache & Prefix State Management (Completed)**
  - Fixed-size physical KV blocks (16 tokens per block) completely eliminating VRAM fragmentation.
  - CPU block table mapping logical tokens to physical VRAM blocks with Copy-On-Write (COW) prefix sharing.
  - **Gate 4 Verification (Passed):** 100% block reuse verified across multi-turn prompts; zero fragmentation and zero leaks.
- [x] **Phase 5: Zero-Overhead Hardware Saturation via CUDA Graphs & Modular ATen Dispatcher (Completed)**
  - Static hardware DAG execution via `cudaStreamBeginCapture` and `cudaGraphLaunch`.
  - Measured host driver launch latency: **$12.9\text{ }\mu\text{s}$** ($0.0129\text{ ms} < 0.05\text{ ms}$ threshold).
  - Modular C++ ATen Dispatcher with zero-copy views and reverse-mode autograd tape achieving 100% numerical parity (0.0 max diff).
  - **Gate 5 Verification (Passed):** Sub-microsecond launch latency and analytical gradient parity verified.
- [x] **Phase 6: Sparse Mixture of Experts (MoE) & Multimodal Output Codecs (Completed)**
  - Partitioned dense SwiGLU FFN into $N=8$ discrete expert weight stacks in contiguous VRAM.
  - Top-2 gating router: $G(x) = \text{TopK}(\text{Softmax}(x W_g), k=2)$ dispatching tokens exclusively to top 2 experts; idle experts consume zero FLOPs.
  - Full-duplex Multimodal Output Codecs (`CompressImage` for 24-bit uncompressed `.bmp` and `CompressAudio` for 16-bit linear PCM `.wav`).
  - **Gate 6 Verification (Passed):** Top-2 routing load balance verified across 8 active experts (`E0: 64 | E1: 61 | E2: 65 | E3: 73 | E4: 66 | E5: 62 | E6: 63 | E7: 58`), and 100% bit-accurate image/audio roundtrip reconstructions verified.

### Next Engineering Horizons
- **Production Pre-Training & Assembly God Specialization:** Ingest large-scale curated datasets (x86_64, PTX, reverse-engineering corpora) via background prefetchers.
- **Multimodal Video Codec Streamer:** Extend visual token serialization into frame sequences wrapped with container bitstreams.
- **Auxiliary Load Balancing Loss:** Fine-tune router gate dispersion via $\mathcal{L}_{\text{aux}}$ regularization.

---

## 6. How to Build, Test & Run

All commands are executed from the PowerShell terminal inside `c:\Users\Omid\X\Code\PROJECT-AXIOM\core`:

```powershell
# 1. Run the entire automated test suite (314 tests across 11 suites):
cd c:\Users\Omid\X\Code\PROJECT-AXIOM\core
.\test.exe --test

# 2. Recompile the test suite (if test files are modified):
mingw32-make test

# 3. Recompile the production engine (if core files are modified):
mingw32-make app

# 4. Run interactive streaming inference:
.\app.exe --use "The quick brown fox"

# 5. Run model training on ingested corpus:
.\app.exe --train --weights database/models/weights.axiom --vocab database/models/vocab.axiom --lr 0.001

# 6. Bulk ingest and tokenize raw assets:
.\app.exe --bulk --data ../data/corpus/ --archive ../data/archive/
```

### Interactive CLI Hotkeys During Training
While training is running, non-blocking asynchronous keystrokes are supported:
- Press **`[S]`**: Displays live training status, iteration count, loss telemetry, and free VRAM.
- Press **`[R]`**: Dynamically prompts to adjust the learning rate without restarting training.
- Press **`[Q]`**: Safely halts training, atomizes weights to disk, and cleanly releases all GPU handles.

---

## 7. How to Respond to Requests

When responding to the user:
1. Maintain the **Pre-Action Reasoning Gate**: Evaluate *What it is*, *What it does*, *How it works*, and *Why it exists* before acting.
2. Follow the **Low-Latency Output Protocol**: Be concise, telegraphic, use 1-line bullets, output only changed code blocks or diffs, and never reprint large unchanged files.
3. Keep git diffs surgical and minimal.
4. Always verify that all 314 unit tests pass (`.\test.exe --test`) after modifying any code in `core/`.
