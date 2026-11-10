# PROJECT-AXIOM: Systems Architecture & Numerical Stability Protocol

This document establishes the formal engineering specification for numerical stability, gradient backpropagation, multimodal data ingestion, and byte-level fusion within the native C++/CUDA Transformer engine.

---

## 1. Forward vs. Backward Mathematical Coupling

The backward pass cannot be verified in isolation because it inherits its operational state directly from the forward pass. Every backward gradient is the analytical derivative of intermediate operations computed during forward inference:

$$\mathbf{Y} = \mathbf{X} \mathbf{W} \implies \nabla_{\mathbf{W}} \mathcal{L} = \mathbf{X}^T \nabla_{\mathbf{Y}} \mathcal{L}, \quad \nabla_{\mathbf{X}} \mathcal{L} = \nabla_{\mathbf{Y}} \mathcal{L} \mathbf{W}^T$$

- **Poisoned Inputs:** If forward activations ($\mathbf{X}$) underflow, overflow, or lack normalization (e.g., missing RMSNorm scaling), backward kernels compute valid derivatives over poisoned activations, destabilizing parameter updates.
- **Operator Symmetry:** Structural modifications to forward kernels (e.g., transitioning from standard GeLU to gated SwiGLU) require exact analytical derivatives in the backward pass. Mismatched forward-backward operators yield gradient ascent instead of descent.

---

## 2. Deterministic Isolation Pipeline

When training anomalies emerge, execute diagnostics in strict forward-first order to decouple corpus artifacts from kernel defects.

```
       [ Training Anomaly Detected ]
                    │
                    ▼
     [ Step 1: Forward-Only Sanity ] ──── NaN / Inf / > 50.0 ───► Fix forward_pass.cu
                    │
                  Finite
                    ▼
     [ Step 2: Single-Batch Overfit ] ─── Fails to reach 0.0 ───► Fix backward_pass.cu
                    │
              Loss -> 0.00
                    ▼
     [ Step 3: Dataset Sanitization ] ─── Binary / Outliers ───► Clean asset_sample
```

### Phase 1: Forward Pass Telemetry (Zero Weight Updates)
1. Set `learning_rate = 0.0` or bypass optimizer parameter integration.
2. Execute a single forward pass over clean prose input.
3. Inspect activations across intermediate tensors (RMSNorm, Attention logits, SwiGLU gates):
   - **Check for `NaN` / `+Inf`:** Indicates division by zero in Softmax, uninitialized VRAM buffers, or numerical overflow.
   - **Check activation magnitudes:** Transformer activations must remain bounded within $[-5.0, +5.0]$. Values exceeding $\pm 50.0$ signal missing variance scaling ($1 / \sqrt{d_k}$).
   - **Verdict:** If activations explode with frozen weights, the defect exists exclusively in `forward_pass.cu`.

### Phase 2: Single-Batch Overfitting Test (Sterile Environment)
1. **Sterilize Corpus:** Isolate one sentence (e.g., *"The quick brown fox jumps over the lazy dog."*) into a single text file. Remove all other assets.
2. **Disable Shuffling:** Lock the dataset stream offset so the identical sequence window is processed every step.
3. **Train for 100 Steps:** Monitor loss trajectory:
   - **Loss converges toward ~0.00:** Forward and backward kernels are mathematically consistent. Divergence on full datasets originates from corpus defects (e.g., raw binary bytes, malformed UTF-8, or high-entropy static).
   - **Loss plateaus, ascends, or outputs NaN:** The defect exists in kernel backpropagation or optimizer state updates.

### Phase 3: Backward Kernel & Optimizer Inspection
When the single-batch overfit test fails, systematically verify the following invariants in `backward_pass.cu`:
1. **Gradient Sign Invariant:** Ensure negative descent orientation:
   $$\mathbf{W}_{t+1} = \mathbf{W}_t - \eta \cdot \frac{\hat{\mathbf{m}}_t}{\sqrt{\hat{\mathbf{v}}_t} + \epsilon}$$
   A positive sign ($+$) executes gradient ascent, causing exponential weight divergence.
2. **Atomic Accumulation (`atomicAdd` vs. `=`):** Parallel CUDA threads writing to shared gradient buffers (embedding lookup tables, projection layers) must accumulate atomically. Unsynchronized assignments (`=`) cause thread collisions and gradient truncation.
3. **Residual Connection Addition:** Skip-connections require accumulating upstream gradients directly into the identity path:
   $$\nabla_{\mathbf{X}_{\text{in}}} \mathcal{L} = \nabla_{\mathbf{X}_{\text{norm}}} \mathcal{L} + \nabla_{\mathbf{X}_{\text{out}}} \mathcal{L}$$
   Omitting residual accumulation fractures gradient flow across deeper Transformer blocks.
4. **Classification vs. Regression Target Mode:** Discrete token prediction requires Softmax Cross-Entropy ($\nabla_{\mathbf{z}} \mathcal{L} = \mathbf{p} - \mathbf{y}_{\text{one-hot}}$). Ensure `token_ids_active` remains true so kernels do not default to continuous MSE regression against invalid memory.
5. **AdamW State Initialization:** Momentum ($\mathbf{m}$) and second-moment ($\mathbf{v}$) buffers must be explicitly zeroed (`cudaMemsetAsync`) upon allocation to prevent uninitialized VRAM values from corrupting initial updates.

---

## 3. Failure Mode Diagnostic Matrix

| Observed Phenomenon | Primary Root Cause | Verification Target |
| :--- | :--- | :--- |
| **Step 1 Loss $> 400.0$** | Sequence-accumulated Cross-Entropy vs. normalized per-token loss metric. | Verify metric divides accumulated loss by context window length $S$. |
| **Loss explodes at Step 1000** | Token mode flag disabled; regression kernel treats target as continuous bfloat16. | Ensure `token_ids_active` remains true through backward pass. |
| **Instant `NaN` on Step 1** | Uninitialized AdamW momentum buffers or Softmax denominator underflow. | Inspect `cudaMemsetAsync` on optimizer tensors and verify $\epsilon \ge 10^{-8}$. |
| **Loss climbs steadily** | Flipped gradient sign or missing residual gradient accumulation. | Confirm weight subtract operation ($\mathbf{W} - \eta \mathbf{G}$) and verify skip connections. |
| **Loss oscillates violently** | Learning rate too high or high-entropy binary static in corpus. | Reduce learning rate by $5\times$ or inspect corpus for non-text artifacts. |

---

## 4. Verification Commands

Run targeted differential verification to confirm kernel math before launching full corpus training:

```powershell
# 1. Execute CUDA loss & SwiGLU backward verification
cd "core"
.\test_loss.exe

# 2. Execute full regression and integration suite
.\test.exe --test

# 3. Validate convergence on clean text corpus
.\app.exe --bulk 1050 --data asset_sample/text
```

---

## 5. Multimodal Data Ingestion & Signal Fidelity

Corpus quality directly dictates optimization landscape geometry. High-entropy non-linguistic data destabilizes optimizer moments.

### 5.1 Semantic vs. Coordinate-Based Text Formats
- **Plain Text (`.txt`):** Stores discrete linguistic units sequentially in universal encodings (UTF-8). The parser processes unambiguous reading order.
- **Portable Document Format (`.pdf`):** A visual display description rather than a semantic text stream. Glyphs are positioned via 2D coordinate operators (`Tj`, `TJ`). Multi-column layouts, figures, and font encodings often cause extractors to emit interleaved fragments or raw binary font tables. Scanned PDFs without OCR contain zero digital text.
- **Markdown (`.md`):** The optimal format for language modeling. Preserves structural hierarchy (headers `#`, lists `*`, tables `|`, code blocks ```` ````) using standard ASCII/UTF-8 tokens without introducing binary metadata.

### 5.2 Continuous Modalities & Signal Encodings
Unlike discrete symbolic text, visual and acoustic signals are continuous waveforms and spatial fields. The objective is balancing lossless fidelity against decode latency:

1. **Images:**
   - **Lossless PNG:** Preserves exact pixel channel values. Critical for dense high-frequency features (technical diagrams, charts, medical imaging).
   - **JPEG:** Suitable for large-scale web scraping, but aggressive compression introduces blocking artifacts that inject noise into early convolutional and patch-projection layers.
2. **Audio & Voice:**
   - **Uncompressed WAV / Lossless FLAC:** Preserves full harmonic spectrum and phase consistency required for high-fidelity spectrogram computation.
   - **Lossy MP3:** Employs psychoacoustic masking that eliminates high-frequency components humans cannot perceive, but models detect as missing spectral bands, degrading voice synthesis and acoustic modeling.
3. **Video:**
   - **Container & Codec:** MP4 container using the standard **H.264** codec.
   - **Hardware Acceleration:** Enterprise GPUs incorporate dedicated silicon decoders (NVDEC) optimized for parallel H.264 frame extraction, eliminating CPU decode bottlenecks that starve GPU compute pipelines.

### 5.3 High-Throughput Streaming Formats
Direct filesystem operations on millions of loose individual files exhaust OS inode tables and generate extreme random I/O latency. Production pipelines aggregate assets into contiguous streamable containers:

- **JSON Lines (`.jsonl`):** Stores one document per line with associated metadata keys:
  ```json
  {"text": "# Header\nContent...", "source": "manual.md", "domain": "engineering"}
  ```
- **WebDataset (`.tar` Archives):** Shards multimodal tuples into sequential archive blocks (e.g., 100 MB – 2 GB per shard):
  - `sample_001.mp4` (Raw video stream)
  - `sample_001.md` (Transcribed text ground truth)
  - `sample_001.json` (Sensor and acquisition metadata)
  
  Enables sequential streaming from storage directly into host pinned memory and GPU VRAM via direct DMA transfers.

---

## 6. Tokenization vs. Byte-Level Early Fusion

The central design trade-off in modern neural sequence modeling centers on tokenization overhead versus raw byte scaling.

### 6.1 The Tokenizer Dilemma: Semantic Density vs. $O(N^2)$ Complexity
Standard tokenizers map variable-length byte strings to discrete integer vocabularies:

- **Advantages:** Maximizes **semantic density**. A single integer token maps directly to a high-dimensional concept vector, enabling deep Transformer attention over multi-thousand token contexts.
- **Constraints:** Standard self-attention scales quadratically with sequence length $N$:
  $$\text{Complexity} = \mathcal{O}(N^2 \cdot d)$$
  Expanding raw bytes directly into the sequence window explodes computational complexity:
  - Text word: 1 token $\to$ 8–12 bytes ($\approx 100\times$ self-attention compute).
  - Audio: 1 second uncompressed $\to$ 48,000 bytes.
  - Image: $512 \times 512$ RGB $\to$ 786,432 bytes.
  
  Processing uncompressed raw bytes directly through naive self-attention exhausts VRAM capacity and memory bandwidth.

### 6.2 Omnimodal Early Fusion Architecture
To process arbitrary modalities without brittle modality-specific encoders, the engine can implement **Patch-Level Byte Ingestion**:

```
 [ Raw Byte Stream ] ──► [ Fixed-Size Patching (e.g. 256 Bytes) ] ──► [ Linear Projection Layer ] ──► [ Unified Latent Space ]
```

1. **Byte Patching:** Chunk arbitrary incoming memory streams into uniform byte patches (e.g., $P = 256$ bytes) regardless of source modality.
2. **Universal Projection:** Feed the byte patch through a lightweight linear projection layer that maps $P$ raw bytes directly into the model embedding dimension $D$.
3. **Unified Transformer Backbone:** Once projected into latent vectors in VRAM, the Transformer backbone processes all modalities symmetrically as a continuous, unified sequence of representations.
