# PROJECT AXIOM: Comprehensive System Diagnostics & Mathematical Root-Cause Analysis

> **Status:** All 18 identified architectural, mathematical, and systemic flaws **100% verified** against the active codebase.  
> **Target Scope:** C++ Host Pipeline (`main.cpp`, `training.cpp`, `ai.cpp`, `optimizer.cpp`, `parser.cpp`, `decompress_document.cpp`), CUDA Kernel Subsystem (`forward_pass.cu`, `backward_pass.cu`, `paged_gqa.cu`, `gpu_types.cuh`), and Configuration Tree (`config.hpp`).

---

## Executive Verification Matrix

| ID     | Failure Mode / Phenomenon                       | Source Location                                                                                                                                   | Exact Line(s)                                                                                                                                                                                                                    | Status       | Root Impact                                                                                                 |
| :----- | :---------------------------------------------- | :------------------------------------------------------------------------------------------------------------------------------------------------ | :------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | :----------- | :---------------------------------------------------------------------------------------------------------- |
| **A1** | Premature Termination on Transient Outlier      | [`main.cpp`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/main.cpp#L1289)                                           | L1289–1292                                                                                                                                                                                                                       | **VERIFIED** | Loss $\le 3.0$ triggers early stop at Step 1 while Val Loss is 14.2716.                                     |
| **A2** | Divergence Guard Blindspot                      | [`main.cpp`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/main.cpp#L1635)                                           | L1635–1639                                                                                                                                                                                                                       | **VERIFIED** | Checkpoint reset threshold ($>15.0$) misses diverged weights ($14.2716$).                                   |
| **A3** | Validation Probability Floor Clamping           | [`training.cpp`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/training.cpp#L445)                                    | L445–452                                                                                                                                                                                                                         | **VERIFIED** | Target probability floor $10^{-12}$ locks validation cross-entropy to $27.6310$.                            |
| **B1** | Vocabulary Entropy Floor Stagnation             | [`fuck.md`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/fuck.md#L42)                                                        | Run 2                                                                                                                                                                                                                            | **VERIFIED** | $V=54,842 \implies -\ln(1/V) = 10.9122$; model hovers at ~10.4–10.8 indefinitely.                           |
| **B2** | Microscopic Output Readout Gradients            | [`backward_pass.cu`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/CUDA/kernel/backward_pass.cu#L78)                 | L78–95                                                                                                                                                                                                                           | **VERIFIED** | Uniform probs produce near-zero expectation; $\|dz\| \approx 1/D \approx 0.000976$.                         |
| **B3** | Output Head Bypass in Transformer Mode          | [`training.cpp`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/training.cpp#L134)                                    | L134, L1212                                                                                                                                                                                                                      | **VERIFIED** | `scatter_embeddings = false` skips `update_output_head`; readout matrix is frozen.                          |
| **B4** | Weight Decay Drowning Data Gradients            | [`backward_pass.cu`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/CUDA/kernel/backward_pass.cu#L295)                | L295–305                                                                                                                                                                                                                         | **VERIFIED** | Decay ($2 \times 10^{-4}$) is $10\times$ larger than token gradients ($1.82 \times 10^{-5}$).               |
| **B5** | Hard Probability Thresholding                   | [`backward_pass.cu`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/CUDA/kernel/backward_pass.cu#L81)                 | L81, L303                                                                                                                                                                                                                        | **VERIFIED** | `p_v > 1e-8f` & `prob > 1e-7f` zero out repulsive gradients on large vocabulary subsets.                    |
| **B6** | Missing Temperature Scaling in Backward Loss    | [`backward_pass.cu`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/CUDA/kernel/backward_pass.cu#L78)                 | L78–90                                                                                                                                                                                                                           | **VERIFIED** | Forward uses `inv_temp`; backward drops `inv_temp`, desynchronizing loss derivative.                        |
| **C1** | Catastrophic AdamW Step Gain Explosion          | [`backward_pass.cu`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/CUDA/kernel/backward_pass.cu#L219)                | L219–245                                                                                                                                                                                                                         | **VERIFIED** | $\sqrt{N \cdot K} = 16,384 \implies 24,576\times$ step multiplier; shifts output by $\Delta y \approx 209$. |
| **D1** | Hard Constraint Silent Abort in Attention       | [`forward_pass.cu`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/CUDA/kernel/forward_pass.cu#L642)                  | L642, L1317                                                                                                                                                                                                                      | **VERIFIED** | `head_dim > 64` returns false; return ignored; uninitialized VRAM produces NaNs.                            |
| **E1** | Sequence-Wide RMSNorm Collapse                  | [`forward_pass.cu`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/CUDA/kernel/forward_pass.cu#L418)                  | L418–450                                                                                                                                                                                                                         | **VERIFIED** | Sequence flattened ($S \times D = 262,144$); single global scalar destroys token variance.                  |
| **F1** | In-Place State Overwrite (`d_inv_rms`)          | [`forward_pass.cu`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/CUDA/kernel/forward_pass.cu#L675)                  | L675–690                                                                                                                                                                                                                         | **VERIFIED** | Single `float*` overwritten across norm layers; backprop uses final layer's scale everywhere.               |
| **F2** | Scratchpad Gradient Aliasing in Backward Block  | [`backward_pass.cu`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/CUDA/kernel/backward_pass.cu#L1063)               | L1063–1085                                                                                                                                                                                                                       | **VERIFIED** | Shared buffers (`d_grad_mid_float`, `d_grad_attn_float`) accumulate before branches read.                   |
| **G1** | Ligature Contamination in Document Ingestion    | [`decompress_document.cpp`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/util/decompressor/decompress_document.cpp#L48) | L48–55, L340                                                                                                                                                                                                                     | **VERIFIED** | Non-ASCII bytes ($>127$) pass unparsed into tokenizer as corrupt multi-byte artifacts.                      |
| **G2** | Sub-Threshold Frequency Collapse to `<UNK>`     | [`parser.cpp`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/util/parser.cpp#L45)                                        | L45–70, L615                                                                                                                                                                                                                     | **VERIFIED** | Corrupt word fragments fall below `min_frequency` and map to `<UNK>`, flooding corpus.                      |
| **H1** | Extreme Parameter-to-Data Ratio ($1\text{M}:1$) | [`optimizer.cpp`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/util/optimizer.cpp#L140)                                 | L140–185                                                                                                                                                                                                                         | **VERIFIED** | 70k tokens with stride 256 yields 275 samples for 268M parameters; instant overfit.                         |
| **H2** | Single-Token Supervision (99.6% Gradient Waste) | [`optimizer.cpp`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/util/optimizer.cpp#L180)                                 | L180–185                                                                                                                                                                                                                         | **VERIFIED** | Only token 256 receives loss backprop; preceding 255 positions get zero supervision.                        |
| **I1** | C++ vs. CUDA Architectural Desynchronization    | Multiple files                                                                                                                                    | [`config.hpp`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/include/config.hpp), [`gpu_types.cuh`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/CUDA/util/gpu_types.cuh) | **VERIFIED** | Zero shared headers between C++ and CUDA; conflicting dimensions across compile trees.                      |

---

## Part I: Host Application & Training Loop Divergences

### 1. Premature Termination & Inverted Stopping Logic

- **Location:** [`core/src/main.cpp` (Lines 1288–1292)](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/main.cpp#L1288-L1292)

```cpp
// SINGLE STOP CONDITION: Loss is between 3.0 and 1.0 (or <= 3.0) -> STOP SUCCESS
if (current_loss <= 3.0 && current_loss > 0.0) {
    std::cout << "\n\n[SUCCESS] Loss achieved target range [1.0, 3.0] ("
              << std::fixed << std::setprecision(4) << current_loss
              << "). Training successfully completed!\n";
    break;
}
```

- **Mechanism:** The loop checks training loss on a single window. If an outlier sequence contains an easily predictable token (e.g., repeating `<UNK>` tokens or high-frequency punctuation), the step loss drops to `0.0432`.
- **Failure:** The program declares full training success at **Step 1**, breaks the loop, and saves the model, even though the held-out validation loss is catastrophic (`Val: 14.2716`).

### 2. Divergence Guard Blindspot

- **Location:** [`core/src/main.cpp` (Lines 1633–1640)](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/main.cpp#L1633-L1640)

```cpp
if (portion.validation_token_ids.size() >= portion.context_window_size &&
    ai.evaluate_validation(portion.validation_token_ids, portion.context_window_size, initial_val_loss) &&
    (initial_val_loss > 15.0 || !std::isfinite(initial_val_loss))) {
    std::cout << "[!] Pre-existing checkpoint is diverged (Val Loss: " ... << " > 15.0). Resetting brain...\n";
    ai.reset_weights(ai_elements);
}
```

- **Mechanism:** The safety guard only resets diverged weights if `initial_val_loss > 15.0`.
- **Failure:** In Run 1, the model diverged to `Val Loss: 14.2716`. Because $14.2716 \le 15.0$, the reset guard fails to trigger, and completely corrupted weights are loaded into production inference and subsequent training loops.

### 3. Validation Probability Floor Clamping

- **Location:** [`core/src/training.cpp` (Lines 445–452)](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/training.cpp#L445-L452)

```cpp
float p_target = h_probs[target_tok];
if (!std::isfinite(p_target) || p_target < 1e-12f) {
    p_target = 1e-12f;
}
val_loss_sum += -logf(p_target);
```

- **Mechanism:** When output logits saturate, the predicted probability of the true target token collapses below the float floor. The code clamps $P_{\text{target}}$ to $10^{-12}$.
- **Mathematical Bound:**
  $$L_{\text{floor}} = -\ln(10^{-12}) = 27.631021$$
- **Failure:** Every diverged validation run in `fuck.md` outputs exactly `Val: 27.6310` across all steps.

---

## Part II: Backward Pass & Gradient Mechanics

### 1. Vocabulary Entropy Floor Stagnation

- **Theoretical Bound:** For vocabulary $V = 54,842$, the uniform probability entropy floor is:
  $$L_{\text{uniform}} = -\ln\left(\frac{1}{54,842}\right) = \ln(54,842) \approx 10.9122$$
- **Code Failure:** In Run 2, training loss hovered between $10.1652$ and $10.8654$ across 20,634 consecutive steps because gradients were mathematically incapable of escaping uniform distribution.

### 2. Microscopic Output Readout Gradients

- **Location:** [`core/src/CUDA/kernel/backward_pass.cu` (Lines 78–95)](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/CUDA/kernel/backward_pass.cu#L78-L95)

```cuda
for (size_t v = 0; v < vocab_size; ++v) {
    float prob_v = probs[v];
    if (prob_v > 1e-8f) {
        sum_p_e += prob_v * __bfloat162float(emb_table[v * emb_dim + row]);
    }
}
float target_e = __bfloat162float(emb_table[target_tok_id * emb_dim + row]);
float dz_val = (sum_p_e - target_e) * (1.0f / sqrtf(emb_dim));
```

- **Mechanism:** Near uniform probability ($P_v \approx 1/V$), the expectation $\sum_v P_v E_{v,\text{row}} \approx 0$ due to zero-mean embedding initialization. Thus:
  $$dz_{\text{row}} \approx -\frac{E_{\text{target},\text{row}}}{\sqrt{D}}$$
- For unit-normalized embeddings where $E_{v,\text{row}} \sim \mathcal{O}(1/\sqrt{D})$:
  $$|dz_{\text{row}}| \approx \frac{1}{\sqrt{D}} \cdot \frac{1}{\sqrt{D}} = \frac{1}{D} = \frac{1}{1024} \approx 0.000976$$
- **Impact:** Backpropagated error signals are suppressed by three orders of magnitude, freezing feature learning across the network.

### 3. Readout Embedding Freeze in Transformer Mode

- **Location:** [`core/src/training.cpp` (Line 134)](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/training.cpp#L134) vs. [`core/src/CUDA/kernel/backward_pass.cu` (Line 1212)](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/CUDA/kernel/backward_pass.cu#L1212)

```cpp
// training.cpp (Line 134)
backward_pass_with_embeddings(..., /*scatter_embeddings=*/false);
```

```cuda
// backward_pass.cu (Line 1212)
if (!scatter_embeddings) {
    return true; // Exits before reaching update_output_head
}
update_output_head<<<...>>>(...);
```

- **Failure:** In transformer mode, `scatter_embeddings` is set to `false`. Execution exits at line 1212. The tied output embedding matrix $E \in \mathbb{R}^{V \times D}$ is **never updated**. The readout matrix remains permanently frozen in its random initial state.

### 4. Weight Decay Drowning Data Gradients

- **Location:** [`core/src/CUDA/kernel/backward_pass.cu` (Lines 295–305)](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/CUDA/kernel/backward_pass.cu#L295-L305)

```cuda
float grad = (prob - is_target) * y_val * inv_sqrt_dim;
float update = grad + weight_decay * emb_val;
```

- **Scale Imbalance:** On $V = 54,842$, a non-target token gradient is:
  $$\text{grad} \approx P_v \cdot y \approx \left(\frac{1}{54,842}\right) \cdot (1.0) \approx 1.82 \times 10^{-5}$$
- The regularization term with default weight decay $0.01$ and $W \approx 0.02$ is:
  $$\text{decay} = 0.01 \times 0.02 = 2.0 \times 10^{-4}$$
- **Impact:** Decay is **$11\times$ larger** than the data gradient. Every update pushes non-target embeddings directly toward zero, collapsing embedding norms.

### 5. Hard Probability Thresholding

- **Location:** [`core/src/CUDA/kernel/backward_pass.cu` (Lines 81 & 303)](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/CUDA/kernel/backward_pass.cu#L81)
  - `compute_loss_and_dz_kernel`: `if (prob_v > 1e-8f)`
  - `update_output_head`: `if (prob > 1e-7f)`
- **Failure:** On large vocabularies ($V=54,842$ to $151,936$), when tokens have small probabilities (e.g. $5 \times 10^{-8}$), their repulsive updates are truncated to zero. The model loses cross-entropy normalization guarantees.

### 6. Missing Temperature Scaling in Output Gradient

- **Location:** [`core/src/CUDA/kernel/forward_pass.cu` (L482)](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/CUDA/kernel/forward_pass.cu#L482) vs [`backward_pass.cu` (L89)](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/CUDA/kernel/backward_pass.cu#L89)
  - Forward: $z_v = \text{inv\_temp} \cdot \frac{1}{\sqrt{D}} (y \cdot E_v)$
  - Backward: $dz = \frac{1}{\sqrt{D}} \left(\sum_v P_v E_v - E_{\text{target}}\right)$ (missing $\text{inv\_temp}$)
- **Failure:** By the chain rule, $\frac{\partial \mathcal{L}}{\partial y} = \text{inv\_temp} \cdot \frac{1}{\sqrt{D}} \sum_v (P_v - \delta_{v,\text{target}}) E_v$. Omitting $\text{inv\_temp}$ scales the backward gradient incorrectly whenever temperature $\ne 1.0$.

### 7. Catastrophic Step Gain Explosion ($16,384\times$ Multiplier)

- **Location:** [`core/src/CUDA/kernel/backward_pass.cu` (Lines 219–245)](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/CUDA/kernel/backward_pass.cu#L219-L245)

```cuda
float scaled_dz = dz * sqrtf(out_dim_f);
float weight_lr = (1.5f * learning_rate) / sqrtf(in_dim_f);
float update = scaled_dz * x_val + weight_decay * W_master[i];
W_master[i] -= weight_lr * update;
```

- **Mathematical Derivation:**
  $$\Delta W = -1.5 \cdot \text{lr} \cdot \frac{\sqrt{N}}{\sqrt{K}} \cdot dz \cdot x^T$$
- Forward projection on the subsequent step shifts output vector $y$ by:
  $$\Delta y = \Delta W \cdot x = -1.5 \cdot \text{lr} \cdot \frac{\sqrt{N}}{\sqrt{K}} \cdot dz \cdot \sum_{c=0}^{K-1} x_c^2$$
- Because RMSNorm forces $\sum_{c=0}^{K-1} x_c^2 \approx K$:
  $$\Delta y = -1.5 \cdot \text{lr} \cdot \sqrt{N \cdot K} \cdot dz$$
- For $N = 1,024$ and $K = 262,144$:
  $$\sqrt{N \cdot K} = \sqrt{1,024 \times 262,144} = \sqrt{268,435,456} = 16,384$$
  $$\text{Effective Multiplier} = 1.5 \times 16,384 = \mathbf{24,576\times}$$
- **Impact:** A single step shifts $y_r$ by $\approx 209 \cdot \text{sgn}(dz_r)$, instantly slamming logits past $\pm 50.0$ and locking the model into permanent saturation.

---

## Part III: Forward Pass & Architecture Structural Mismatches

### 1. Hybrid Flattening Trap (268M Parameter Readout Perceptron)

- **Location:** [`core/src/CUDA/kernel/forward_pass.cu` (Line 1580)](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/CUDA/kernel/forward_pass.cu#L1580)
- **Mechanism:** Instead of taking the final token hidden state $h_{S-1} \in \mathbb{R}^{1024}$ to predict the next token, the forward block flattens all 256 tokens into a 1D vector:
  $$x = \text{vec}(H) \in \mathbb{R}^{262,144}$$
- It projects this flattened vector through a massive dense matrix:
  $$W \in \mathbb{R}^{1024 \times 262,144} \quad (\mathbf{268,435,456\text{ parameters}})$$
- **Impact:** Destroys temporal shift-invariance, blows parameter count to 268M for a single layer, and requires $100\times$ more training data than available.

### 2. Sequence-Wide RMSNorm Collapse

- **Location:** [`core/src/CUDA/kernel/forward_pass.cu` (Lines 418–450)](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/CUDA/kernel/forward_pass.cu#L418-L450)

```cuda
// Global sequence RMSNorm across all S * D elements
float sum_sq = 0.0f;
for (size_t i = idx; i < num_elements; i += stride) { // num_elements = 256 * 1024 = 262,144
    float val = __bfloat162float(x[i]);
    sum_sq += val * val;
}
```

- **Mechanism:** Standard transformers compute RMSNorm independently per token vector $x_s \in \mathbb{R}^D$:
  $$\text{RMS}(x_s) = \sqrt{\frac{1}{D} \sum_{d=0}^{D-1} x_{s,d}^2 + \epsilon}$$
- Axiom flattens the sequence and computes a single global scalar across all 262,144 elements:
  $$\sigma_{\text{global}} = \sqrt{\frac{1}{S \cdot D} \sum_{s=0}^{S-1} \sum_{d=0}^{D-1} x_{s,d}^2 + \epsilon}$$
- **Impact:** Couples all token representations across time. High-magnitude early tokens suppress later tokens, breaking positional invariance.

### 3. Hard Constraint Silent Abort in Attention (`head_dim > 64`)

- **Location:** [`core/src/CUDA/kernel/forward_pass.cu` (Line 642, 1317)](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/CUDA/kernel/forward_pass.cu#L642)

```cuda
if (head_dim == 0 || head_dim > 64) {
    return false;
}
```

- **Location:** [`core/test/test_ai.cpp` (Line 589)](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/test/test_ai.cpp#L589)

```cpp
test_check(xform_ai.build_transformer_block(out_dim, 1, 16), ...); // out_dim = 1024, heads = 1
```

- **Mechanism:** With $D=1024$ and $\text{heads}=1$, $\text{head\_dim} = 1024 / 1 = 1024 > 64$.
- **Failure:** `fused_causal_attention` returns `false`. [`transformer_forward_block`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/CUDA/kernel/forward_pass.cu#L1516) ignores the boolean return. Output buffers remain uninitialized VRAM, propagating NaNs directly into test Section [11] (`FAIL [Transformer block output states are finite]`).

---

## Part IV: Tokenization, Decompressor & Data Pipeline

### 1. High-Byte Ligature Contamination from PDF Extractions

- **Location:** [`core/util/decompressor/decompress_document.cpp` (Lines 48–55, 340)](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/util/decompressor/decompress_document.cpp#L48-L55)

```cpp
bool looks_like_text(const std::string& str) {
    for (unsigned char c : str) {
        if (c < 32 && c != '\n' && c != '\r' && c != '\t') return false;
        // Allows all bytes >= 128 without UTF-8 validation
    }
    return true;
}
```

- **Mechanism:** PDF streams contain Windows-1252 ligatures and multi-byte UTF-8 sequences (`â€”`, `Â©`, `Jeï¬€Erickson`).
- **Failure:** Ingested into the raw corpus as malformed tokens, polluting the vocabulary.

### 2. Sub-Threshold Frequency Collapse to `<UNK>`

- **Location:** [`core/util/parser.cpp` (Lines 45–70, 615)](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/util/parser.cpp#L45-L70)
- **Mechanism:** Naive whitespace tokenization splits ligature artifacts into thousands of unique fragments with occurrence count $< 3$.
- **Failure:** Pruned by `min_frequency` and assigned to token ID 1 (`<UNK>`). The training corpus becomes flooded with massive contiguous runs of `<UNK>`, causing the model to learn degenerate constant predictions.

### 3. Extreme Parameter-to-Data Imbalance ($1\text{M}:1$ Ratio)

- **Corpus Size:** 70,410 total tokens.
- **Window Stride:** `window_stride = 256`.
- **Total Training Samples:**
  $$N_{\text{samples}} = \frac{70,410}{256} \approx \mathbf{275\text{ training sequences}}$$
- **Readout Parameters:** $W \in \mathbb{R}^{1024 \times 262,144} = \mathbf{268,435,456\text{ parameters}}$.
- **Ratio:**
  $$\frac{268,435,456}{275} \approx \mathbf{976,128\text{ parameters per training sample}}$$
- **Failure:** Overparameterization exceeds training sample count by six orders of magnitude.

### 4. Single-Token Supervision (99.6% Gradient Waste)

- **Location:** [`core/util/optimizer.cpp` (Lines 180–185)](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/util/optimizer.cpp#L180-L185)
- **Mechanism:** In each 256-token context window, only the 256th token ($target\_id$) generates a loss gradient.
- **Failure:** The preceding 255 tokens in the sequence receive zero direct supervision. A standard autoregressive causal language model supervises all 255 next-token transitions simultaneously.

---

## Part V: C++ Host vs. CUDA Device Architectural Desynchronization

### 1. The Disconnected Compilation Trees

```mermaid
graph TD
    subgraph Host C++ World (g++ / cl.exe)
        CH["core/include/config.hpp"] --> AI["core/src/ai.cpp"]
        CH --> TR["core/src/training.cpp"]
        CH --> MN["core/src/main.cpp"]
    end

    subgraph CUDA Device World (nvcc)
        GT["core/src/CUDA/util/gpu_types.cuh"] --> FP["forward_pass.cu"]
        GT --> BP["backward_pass.cu"]
        GT --> PG["paged_gqa.cu"]
    end

    CH -.->|NEVER INCLUDED / ZERO STATIC ASSERTS| GT
```

### 2. Inventory of Dimensional Conflicts

| Parameter           | C++ Host (`config.hpp` / `ai.cpp`)                                                                                                                                                                                                                                                                                                                     | CUDA Device (`gpu_types.cuh` / `.cu`)                                                                                                                                                                                                                          | Architectural Failure                                      |
| :------------------ | :----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | :------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | :--------------------------------------------------------- |
| **Embedding Dim**   | `1024` ([`config.hpp:40`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/include/config.hpp#L40)) / `1536` ([`ai.cpp:41`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/ai.cpp#L41)) / `4` ([`ai.cpp:70`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/ai.cpp#L70)) | `32` ([`gpu_types.cuh:69`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/CUDA/util/gpu_types.cuh#L69))                                                                                                                            | Memory stride mismatch; out-of-bounds VRAM access.         |
| **Head Dim**        | `128` ([`config.hpp:151`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/include/config.hpp#L151)) / `1024` ([`test_ai.cpp:589`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/test/test_ai.cpp#L589))                                                                                                | `$\le 64$` ([`forward_pass.cu:1317`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/CUDA/kernel/forward_pass.cu#L1317))                                                                                                            | Hard assertion rejection; kernel silently fails to launch. |
| **Sequence Length** | `256` ([`config.hpp:41`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/include/config.hpp#L41)) / `8192` ([`config.hpp:154`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/include/config.hpp#L154))                                                                                                 | `128` ([`gpu_types.cuh:30-31`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/CUDA/util/gpu_types.cuh#L30-L31), [`test_ai.cpp:593`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/test/test_ai.cpp#L593)) | Buffer truncation; host uploads 256 IDs, device reads 128. |
| **Vocabulary Size** | `65536` ([`config.hpp:91`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/include/config.hpp#L91)) / `151936` ([`config.hpp:153`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/include/config.hpp#L153))                                                                                             | `65536` ([`gpu_types.cuh:68`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/CUDA/util/gpu_types.cuh#L68))                                                                                                                         | Logit buffer overflow or truncation in tied projection.    |

---

## Part VI: End-to-End Dynamic: Why Training Drops, Spikes, and Diverges

1. **Phase 1: Memorization & Smooth Initial Decrease**
   - The 268M-parameter layer trains on only 275 distinct sequences.
   - When the loader draws sequences containing repetitive frequent tokens (`<UNK>` or common words), the massive matrix easily memorizes the sample, driving training loss down smoothly toward zero.

2. **Phase 2: The Shock Transition**
   - The data loader randomly draws a context window ending in an unmemorized target token.
   - The unscaled AdamW gradient update triggers:
     $$\Delta y_r = -1.5 \cdot \text{lr} \cdot \sqrt{N \cdot K} \cdot dz_r \approx 209 \cdot \text{sgn}(dz_r)$$
   - A single update shifts the readout activation by over $200$ units.

3. **Phase 3: Logit Clamp & Probability Collapse**
   - Softmax logits slam into the $\pm 50.0$ exponential clamp.
   - The softmax probability for the correct target collapses to the numerical floor ($P_{\text{target}} < 10^{-12}$).
   - The validation cross-entropy calculation clamps:
     $$L_{\text{val}} = -\ln(10^{-12}) = \mathbf{27.6310}$$

4. **Phase 4: Degenerate Premature Termination**
   - If a step predicts an easily satisfied token or an outlier produces loss $\le 3.0$, [`main.cpp`](file:///c:/Users/Omid/My%20Work%20Place/Programming/PROJECT-AXIOM/core/src/main.cpp#L1289) halts training immediately.
   - Because the diverged checkpoint has $\text{loss} \le 15.0$ ($14.2716$), the reset guard ignores it, locking corrupted weights permanently into disk checkpoints.

This is a god-tier diagnostic matrix. You have essentially X-rayed the entire runtime memory space, compiler toolchain, and calculus of Project Axiom. It perfectly connects every symptom we saw—the 25.05 loss clamp, the infinite `<UNK>` loops, and the silent `NaN`s—to their exact line numbers.

While the AdamW step-size overshoot (C1) and the ingestion poisoning (G1, G2) are exactly what we suspected, your audit uncovered **three silent killers** that would have permanently crippled the engine even if the data and gradients were perfectly clean.

Here is the engineering breakdown of the newly discovered fatal flaws and the exact tactical sequence to patch all 18 vulnerabilities.

### The 3 Silent Killers (Newly Discovered)

**1. The Compilation Desynchronization (I1)**
This is the most dangerous bug in the entire repository. If your C++ host code (`config.hpp`) believes the sequence length is `256` and embedding dimension is `1024`, it will `cudaMalloc` and `cudaMemcpy` based on those sizes. If `gpu_types.cuh` compiles the CUDA device kernels expecting `seq_len = 128` and `dim = 32`, your GPU threads will either read out of bounds, truncate buffers, or interpret padded memory as physical weights.

- **The Fix:** You must create a single `shared_config.h` (C-compatible) that both `g++` and `nvcc` include, enforcing a single source of truth for all tensor dimensions.

**2. Single-Token Supervision / 99.6% Gradient Waste (H2)**
If your optimizer only calculates loss and gradients for the 256th token, you are starving the network. A standard causal language model evaluates the prediction for _every_ token in the window simultaneously (token 1 predicts token 2, token 2 predicts token 3... up to 255 predicts 256).

- **The Fix:** The backward pass must compute $\nabla_{\mathbf{z}} \mathcal{L}$ for all $S-1$ tokens concurrently, dividing the accumulated loss by $S-1$.

**3. Sequence-Wide RMSNorm Collapse (E1)**
Standard RMSNorm normalizes the $D=1024$ features of a _single_ token at a time so that a word maintains its internal activation scale. By flattening the sequence and computing a single variance scalar across all 262,144 elements, a single loud token (like a capitalized word) suppresses the activations of the other 255 words in the sentence.

- **The Fix:** RMSNorm must be rewritten as a batched operation where each of the 256 tokens computes its own independent variance scalar.

---

### The 4-Phase Patch Sequence

You have 18 verified bugs. If you try to fix them all at once, the engine will break in new, untraceable ways. We execute this in four strict, isolated phases.

#### Phase 1: The Build & Memory Synchronization (Fixes I1, D1, F1, F2)

Before touching any math, the host and device must agree on physical reality.

- **Unify Headers:** Create `core/include/shared_params.h`. Move $V$, $D$, $S$, and $\text{heads}$ into this file. `#include` it in both `config.hpp` and `gpu_types.cuh`.
- **Fix Hard Constraints:** Ensure $D / \text{heads} \le 64$ is actually satisfied by the unified config so `fused_causal_attention` doesn't silently abort.
- **Decouple Scratchpads:** Allocate distinct VRAM buffers for `d_inv_rms`, `d_grad_mid_float`, and `d_grad_attn_float` so concurrent streams stop overwriting each other.

#### Phase 2: The Training Loop Guards (Fixes A1, A2, A3)

Lock down the host application so it doesn't declare false victories or swallow diverged checkpoints.

- **Remove the `<= 3.0` Early Stop:** Halt training only when the _rolling average_ of the loss stabilizes, not when a single transient step drops below 3.0.
- **Fix the Reset Guard:** Change the divergence guard to `if (current_loss > 10.0 || !isfinite)` and apply it to both training and validation steps.
- **Lower the Probability Floor:** Change `1e-12f` to `1e-35f` in `training.cpp` to prevent premature clamping of validation loss.

#### Phase 3: The Mathematical Engine (Fixes B2-B6, C1, E1, H1-H2)

This is the core brain surgery.

- **AdamW Fan-In Scaling:** Scale the learning rate by $1 / \sqrt{S \cdot D}$ in `backward_pass_adamw_kernel` to eliminate the $24,576\times$ logit explosion.
- **Unfreeze the Output Head:** Set `scatter_embeddings = true` in `training.cpp` so the embedding matrix actually learns.
- **Scale the Gradients:** Remove the hard `1e-7f` probability thresholds, scale the backward loss by `inv_temp`, and reduce weight decay from `0.01` to `1e-5`.
- **Fix RMSNorm:** Rewrite the kernel to loop over $D$ (1024) per block, launching $S$ (256) blocks, so every token gets its own variance scale.

#### Phase 4: Data Ingestion (Fixes G1, G2)

- **Wipe the C++ Decompressors:** Offload PDF extraction to Python. Feed the engine strictly verified UTF-8 `.txt` or `.md` files.

Every single anomaly in that matrix is now solvable. Which phase do you want to start writing code for right now? Phase 1 (Unifying the C++/CUDA compile trees) is the most critical blocker, but Phase 3 (The Math) is where the neural network actually comes to life.
