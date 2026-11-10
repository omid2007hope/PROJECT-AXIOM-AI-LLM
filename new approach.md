# PROJECT-AXIOM: Numerical Stability & Gradient Diagnostic Protocol

This protocol establishes a deterministic framework for diagnosing loss divergence, gradient explosion, and data corruption in the native C++/CUDA Transformer engine.

---

## 1. Forward vs. Backward Mathematical Coupling

The backward pass cannot be debugged in isolation because it inherits its numerical state directly from the forward pass. Every backward gradient is the calculus derivative of operations evaluated during forward inference:

$$\mathbf{Y} = \mathbf{X} \mathbf{W} \implies \nabla_{\mathbf{W}} \mathcal{L} = \mathbf{X}^T \nabla_{\mathbf{Y}} \mathcal{L}, \quad \nabla_{\mathbf{X}} \mathcal{L} = \nabla_{\mathbf{Y}} \mathcal{L} \mathbf{W}^T$$

- **Poisoned Inputs:** If intermediate activations ($\mathbf{X}$) in the forward pass overflow, underflow, or lack normalization (e.g., missing RMSNorm scale factor), the backward pass will compute exact gradients for mathematically corrupted activations.
- **Operator Symmetry:** Any structural change to forward kernels (e.g., transitioning from standard GeLU to gated SwiGLU) mandates an exact analytical derivative in the backward kernel. Mismatched forward-backward operators cause gradient ascent instead of descent.

---

## 2. Deterministic Isolation Pipeline

When training destabilizes, execute diagnostics in strict forward-first order to decouple data anomalies from kernel bugs.

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
1. Set `learning_rate = 0.0` or bypass optimizer weight integration.
2. Execute a single forward pass over clean prose input.
3. Inspect activations across intermediate tensors (RMSNorm output, Attention logits, SwiGLU gates):
   - **Check for `NaN` / `+Inf`:** Indicates division by zero in Softmax, uninitialized VRAM buffers, or numerical overflow.
   - **Check activation magnitudes:** Normal transformer activations remain bounded within $[-5.0, +5.0]$. Values exceeding $\pm 50.0$ signal missing normalization scaling ($1 / \sqrt{d_k}$).
   - **Verdict:** If activations explode with frozen weights, the defect exists exclusively within `forward_pass.cu`.

### Phase 2: Single-Batch Overfitting Test (Sterile Environment)
1. **Sterilize Corpus:** Isolate exactly one short sentence (e.g., *"The quick brown fox jumps over the lazy dog."*) into a single text file. Remove all other assets.
2. **Disable Shuffling:** Lock dataset offset so the identical sequence window is presented each step.
3. **Train for 100 Steps:** Observe loss trajectory.
   - **Loss converges toward ~0.00:** Forward and backward kernels are mathematically consistent. Gradient descent holds. Divergence on full datasets stems from data corruption (e.g., compressed binary assets, invalid UTF-8 sequences, or high-entropy static).
   - **Loss plateaus, ascends, or outputs NaN:** The defect exists in kernel backpropagation or optimizer state updates.

### Phase 3: Backward Kernel & Optimizer Inspection
When the single-batch overfit test fails, systematically inspect these critical failure points in `backward_pass.cu`:
1. **Gradient Sign Invariant:** Ensure descent orientation:
   $$\mathbf{W}_{t+1} = \mathbf{W}_t - \eta \cdot \hat{\mathbf{m}}_t / (\sqrt{\hat{\mathbf{v}}_t} + \epsilon)$$
   A positive sign ($+$) executes gradient ascent, causing immediate exponential divergence.
2. **Atomic Accumulation (`atomicAdd` vs. `=`):** Parallel CUDA threads writing to shared gradient buffers (such as embedding lookup tables or projection layers) must accumulate atomically. Unsynchronized assignments (`=`) overwrite parallel thread contributions with arbitrary thread noise.
3. **Residual Connection Addition:** Skip-connections require accumulating upstream gradients directly into the identity stream:
   $$\nabla_{\mathbf{X}_{\text{in}}} \mathcal{L} = \nabla_{\mathbf{X}_{\text{norm}}} \mathcal{L} + \nabla_{\mathbf{X}_{\text{out}}} \mathcal{L}$$
   Omitting residual accumulation fractures backpropagation across deep layers.
4. **Classification vs. Regression Loss Mode:** Discrete token targets require Softmax Cross-Entropy ($\nabla_{\mathbf{z}} \mathcal{L} = \mathbf{p} - \mathbf{y}_{\text{one-hot}}$). Ensure token mode remains active so kernels do not default to Mean Squared Error regression against out-of-bounds memory.
5. **AdamW State Initialization:** Momentum ($\mathbf{m}$) and second-moment ($\mathbf{v}$) buffers must be deterministically zeroed (`cudaMemsetAsync`) upon allocation to prevent stale VRAM values from distorting initial steps.

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
