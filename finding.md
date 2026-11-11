A. Premature Termination & Inverted Stopping Logic (main.cpp)
Mechanism: In main.cpp, the convergence check evaluates if (current_loss <= 3.0 && current_loss > 0.0).
Failure Cause: A single outlier sequence or degenerate zero-entropy step producing a transient loss of
0.0432
0.0432 triggers an immediate [SUCCESS] state at Step 1, halting training while held-out validation remains catastrophic (Val: 14.2716).
Divergence Guard Blindspot: Line 1636 checks if (initial_val_loss > 15.0) to trigger an emergency weight reset. When validation diverges to
14.2716
14.2716, it falls immediately beneath the reset threshold, allowing corrupt model weights to persist.
B. Vocabulary Entropy Floor Stagnation (fuck.md)
Mathematical Bound: For a vocabulary of
𝑉
=
54
,
842
V=54,842, uniform probability yields a theoretical entropy floor of:
𝐿
uniform
=
−
ln
⁡
(
1
54842
)
=
10.9122
L
uniform
​

=−ln(
54842
1
​

)=10.9122
Run 2 logged 20,634 consecutive steps hovering between
10.1652
10.1652 and
10.8654
10.8654 because the network never escaped uniform distribution.
Microscopic Readout Gradient: In compute_loss_and_dz_kernel, the backpropagated error vector is computed as:
𝑑
𝑧
row
=
1
𝐷
(
∑
𝑣
=
0
𝑉
−
1
𝑃
𝑣
𝐸
𝑣
,
row
−
𝐸
target
,
row
)
dz
row
​

=
D
​

1
​

(
v=0
∑
V−1
​

P
v
​

E
v,row
​

−E
target,row
​

)
With near-uniform distribution (
𝑃
𝑣
≈
1
/
𝑉
P
v
​

≈1/V), the expectation
∑
𝑣
𝑃
𝑣
𝐸
𝑣
≈
0
∑
v
​

P
v
​

E
v
​

≈0, leaving
𝑑
𝑧
≈
−
𝐸
target
/
𝐷
dz≈−E
target
​

/
D
​

. For normalized embeddings (
𝐸
𝑣
,
row
∼
𝑂
(
1
/
𝐷
)
E
v,row
​

∼O(1/
D
​

)), the gradient magnitude drops to
∣
𝑑
𝑧
∣
≈
1
/
𝐷
=
1
/
1024
≈
0.000976
∣dz∣≈1/D=1/1024≈0.000976.
Weight Decay Drowning: In update_output_head, the gradient update term
𝑃
𝑣
⋅
𝑦
P
v
​

⋅y for incorrect tokens is
≈
1.82
×
10
−
5
≈1.82×10
−5
, whereas the weight decay term (
0.01
×
weight
≈
2.0
×
10
−
4
0.01×weight≈2.0×10
−4
) is over
10
×
10× larger. The embedding table decays toward zero faster than data gradients can pull it.
Hard Probability Thresholding: The conditions if (p_v > 1e-8f) in compute_loss_and_dz_kernel and if (prob > 1e-7f) in update_output_head truncate repulsive gradient updates across large vocabulary subsets once probabilities dip below the cutoff.
Output Head Bypass During Transformer Execution: In training.cpp, the transformer training branch invokes backward_pass_with_embeddings with scatter_embeddings = false. Consequently, update_output_head is never invoked, leaving the readout embedding matrix completely frozen while the underlying transformer hidden state
𝑦
y drifts.
C. Catastrophic Step Gain Explosion (
16
,
384
×
16,384× Multiplier)
Mathematical Mechanism: In backward_pass_adamw_kernel:
Δ
𝑊
=
−
1.5
⋅
lr
⋅
𝑁
𝐾
⋅
𝑑
𝑧
⋅
𝑥
𝑇
ΔW=−
K
​

1.5⋅lr⋅
N
​

    ​

⋅dz⋅x
T
The input tensor
𝑥
∈
𝑅
𝐾
x∈R
K
(
𝐾
=
262
,
144
K=262,144) is normalized by RMSNorm to unit root-mean-square per element, establishing:
∥
𝑥
∥
2
2
=
∑
𝑐
=
0
𝐾
−
1
𝑥
𝑐
2
≈
𝐾
=
262
,
144
∥x∥
2
2
​

=
c=0
∑
K−1
​

x
c
2
​

≈K=262,144
When forward-projected on the subsequent step, the change in output
Δ
𝑦
Δy equals:
Δ
𝑦
=
Δ
𝑊
⋅
𝑥
=
−
1.5
⋅
lr
⋅
𝑁
𝐾
⋅
𝑑
𝑧
⋅
∑
𝑐
=
0
𝐾
−
1
𝑥
𝑐
2
=
−
1.5
⋅
lr
⋅
𝑁
⋅
𝐾
⋅
𝑑
𝑧
Δy=ΔW⋅x=−
K
​

1.5⋅lr⋅
N
​

    ​

⋅dz⋅
c=0
∑
K−1
​

x
c
2
​

=−1.5⋅lr⋅
N⋅K
​

⋅dz
For
𝑁
=
1024
N=1024 and
𝐾
=
262
,
144
K=262,144, the product
𝑁
⋅
𝐾
=
268
,
435
,
456
=
16
,
384
N⋅K
​

=
268,435,456
​

=16,384.
Dynamic Impact: Multiplied by
1.5
1.5, this injects an unscaled
24
,
576
×
24,576× step multiplier into the forward readout vector
𝑦
y. Softmax logits overshoot into extreme saturation (
Δ
𝑧

> 50.0
> Δz>50.0), tripping the exponential clamp in softmax_argmax_kernel and collapsing non-target token probabilities to the numerical floor.
> Validation Probability Floor Saturation (
> 27.6310
> 27.6310): In training.cpp, validation cross-entropy clamps target probabilities to
> 10
> −
> 12
> 10
> −12
> :
> 𝐿

# floor

−
ln
⁡
(
10
−
12
)
=
27.631021
L
floor
​

=−ln(10
−12
)=27.631021
Every saturated validation step outputs exactly
27.6310
27.6310.
D. Hard Constraint Failure in Causal Attention Kernel (forward_pass.cu)
Failure Cause: In fused_causal_attention, execution aborts if head_dim > 64:
cuda
if (head_dim > 64) {
return false;
}
Silent NaN Propagation: In transformer_forward_block, the boolean return of fused_causal_attention is ignored.
Test Suite Crash Mechanism: test_ai.cpp instantiates build_transformer_block(out_dim=1024, heads=1, ffn=16), resulting in head_dim = 1024 / 1 = 1024 > 64. The kernel refuses to launch, leaving d_attention_output uninitialized. Subsequent GEMM projections and SwiGLU activations process uninitialized VRAM, generating NaNs and producing the failure observed in Section [11] of fuck.md (FAIL [Transformer block output states are finite]).
E. Sequence-Wide Normalization Collapse (forward_pass.cu)
Mathematical Flaw: rms_norm_forward_kernel accepts num_elements = seq_len \* dim (
256
×
1024
=
262
,
144
256×1024=262,144) and calculates a single sequence-wide scalar:
𝜎
=
1
𝑆
⋅
𝐷
∑
𝑠
=
1
𝑆
∑
𝑑
=
1
𝐷
𝑥
𝑠
,
𝑑
2

- 𝜖
  σ=
  S⋅D
  1
  ​

s=1
∑
S
​

d=1
∑
D
​

x
s,d
2
​

+ϵ
​

Architectural Impact: Standard Transformer RMSNorm must normalize each token
𝑥
𝑠
∈
𝑅
𝐷
x
s
​

∈R
D
independently. Computing a global RMS across all tokens destroys relative inter-token variance: high-energy prompt tokens suppress the activation magnitudes of downstream tokens prior to attention projection.
F. In-Place Gradient Pointer Aliasing & State Overwrite (backward_pass.cu)
State Overwrite: Only a single float\* d_inv_rms exists on gpu_activations. During forward execution, it is overwritten consecutively across Pre-Attention RMSNorm, Post-Attention RMSNorm, and Final RMSNorm. Backpropagation utilizes the single final scalar across all preceding backward normalizers.
Buffer Aliasing: In transformer_backward_block, intermediate gradients d_grad_residual_1 and d_grad_attn_in are accumulated directly into d_grad_input before preceding residual branches consume their respective input signals, causing gradient corruption
Scenario 1: Fundamental Flaws in Backward Pass

Frozen Readout Embeddings During Transformer Training:

Where: AI::train invokes backward_pass_with_embeddings with parameter scatter_embeddings = false.
Why: When scatter_embeddings is false, execution exits at line 1212 without calling update_output_head. The tied embedding matrix
𝐸
∈
𝑅
𝑉
×
𝐷
E∈R
V×D
never receives backpropagated gradients from output logits during transformer runs; it remains completely frozen while only the input layer receives gradients.

Missing Temperature Scaling in Output Loss Gradient:

Where: compute_loss_and_dz_kernel in contrast to compute_tied_logits_kernel.
Why: The forward pass computes logits as
𝑧
𝑣
=
inv_temp
⋅
1
𝐷
(
𝑦
⋅
𝐸
𝑣
)
z
v
​

=inv_temp⋅
D
​

1
​

(y⋅E
v
​

). By the chain rule,
∂
𝐿
∂
𝑦
=
inv_temp
⋅
1
𝐷
(
∑
𝑣
𝑃
𝑣
𝐸
𝑣
−
𝐸
target
)
∂y
∂L
​

=inv_temp⋅
D
​

1
​

(∑
v
​

P
v
​

E
v
​

−E
target
​

). The backward kernel drops inv_temp entirely, causing an immediate gradient scale mismatch whenever temperature
≠
1.0

=1.0.

Weight Decay Drowning Data Gradients in Output Head:

Where: update_output_head.
Why: On large vocabularies (
𝑉
=
54
,
842
V=54,842), typical gradient magnitudes (
𝑃
𝑣
⋅
𝑦
𝑑
≈
1.82
×
10
−
5
P
v
​

⋅y
d
​

≈1.82×10
−5
) are
10
×
10× smaller than the weight decay regularization term (
0.01
×
𝑊
≈
2
×
10
−
4
0.01×W≈2×10
−4
). The readout table continuously decays toward zero rather than learning token semantics.

Massive AdamW Step-Gain Amplification on Flattened Readout:

Where: backward_pass_adamw_kernel.
Why: AdamW updates each coordinate of
𝑊
∈
𝑅
1024
×
262
,
144
W∈R
1024×262,144
by
≈
±
𝛼
≈±α. When projected across
𝐾
=
262
,
144
K=262,144 inputs, the change in the readout vector
Δ
𝑦
𝑟
=
∑
𝑐
Δ
𝑊
𝑟
,
𝑐
𝑥
𝑐
Δy
r
​

=∑
c
​

ΔW
r,c
​

x
c
​

reaches
≈
209
⋅
sgn
(
𝑑
𝑧
𝑟
)
≈209⋅sgn(dz
r
​

) in a single step, instantly forcing downstream logits into the
±
50.0
±50.0 clamp.
Scenario 2: Forward Pass & Architectural Structural Flaws

Hybrid Architecture Flattening (Sequence Collapse into 268M Parameter MLP):

Where: forward_pass_with_embeddings and transformer_forward_block.
Why: Standard autoregressive transformers use the final token state
ℎ
255
∈
𝑅
1024
h
255
​

∈R
1024
to predict token 256. Project Axiom flattens the entire
256
×
1024
256×1024 sequence into a
262
,
144
262,144-dimensional 1D vector and multiplies it by a giant dense matrix
𝑊
∈
𝑅
1024
×
262
,
144
W∈R
1024×262,144
(268,435,456 parameters). This destroys the shift-invariance of the transformer and creates an extreme parameter-to-data mismatch.

Global Sequence RMSNorm Destroying Position Activation Scales:

Where: rms_norm_forward_kernel.
Why: The normalization computes a single scalar RMS value across all 262,144 elements of the sequence rather than normalizing each 1,024-dimensional token vector independently. High-energy tokens in early positions artificially suppress token representations in later positions.

Silent Attention Rejection on Unchecked Head Dimensions:

Where: fused\*causal_attention.
Why: The kernel explicitly enforces if (head_dim > 64) return false;. When build_transformer_block is configured with num_heads = 1 (
ℎ
𝑒
𝑎
𝑑

- 𝑑
  𝑖
  𝑚
  =
  1024
  head_dim=1024), the kernel aborts silently without setting output buffers, propagating uninitialized memory or NaNs downstream (directly responsible for the test failure in test_ai.cpp Section [11]).
  Scenario 3: Decompressor & Tokenizer Pipeline

High-Byte Ligature Contamination from PDF Extractions:

Where: DecompressDocument::decompress_text and looks_like_text.
Why: The decompressor accepts high-byte characters (

> 127 127) without full UTF-8 validation or Windows-1252 to ASCII transliteration. PDF text streams containing ligatures emit multi-byte sequences (e.g., â€”, Â©, Jeï¬€Erickson) directly into the raw text stream.

Sub-Threshold <UNK> Collapse:

Where: sanitize_word and bulk_tokenize_directory.
Why: Naive whitespace tokenization fails to clean multi-byte fragments. When the vocabulary frequency filter is applied, thousands of malformed or rare ligature tokens fall below min_frequency and map to token ID 1 (<UNK>), producing contiguous runs of identical <UNK> tokens across the dataset.
Scenario 4: Data Feeding, Window Stride, and Target Alignment

Extreme Parameter-to-Data Overfitting Ratio (
1
M
:
1
1M:1):

Where: Optimizer::load_context_window and Config.
Why: On a 70,410-token dataset with window_stride = 256, there are only
70
,
410
/
256
≈
275
70,410/256≈275 unique training samples. The model allocates 268.4 million weights for the readout layer alone—over 976,000 parameters per training sequence.

Single-Token Supervision per 256-Token Sequence:

Where: load\*context_window.
Why: The loss is computed solely against the 256th token (
𝑡
𝑎
𝑟
𝑔
𝑒
𝑡

- 𝑖
  𝑑
  target_id). The preceding 255 tokens in the sequence receive zero loss signal, discarding 99.6% of available supervisory gradients.

Mechanism of the "Gradual Decrease, Sudden Spike, and Collapse" Dynamic:

Why:
With 268M parameters memorizing 275 samples, when the randomly shuffled context window hits consecutive windows with frequent targets (<UNK> or common words), the loss drops smoothly.
As soon as the loader draws a window containing an unseen or rare target token, the unscaled AdamW step shifts
Δ
𝑦
𝑟
Δy
r
​

by
209
×
209×.
Downstream logits instantly slam into the
±
50.0
±50.0 saturation clamp, softmax probability for the correct token collapses to
10
−
12
10
−12
, validation loss locks at
−
ln
⁡
(
10
−
12
)
=
27.6310
−ln(10
−12
)=27.6310, and training diverges.

Your gut feeling is 100% verified by the codebase. The project currently has no shared single source of truth between C++ and CUDA. Host and device compilers live in two disconnected worlds with conflicting magic numbers and hardcoded variables:

A. Direct C++ vs. CUDA Dimensional Desynchronizations

Embedding Dimension Chaos:

Where:
gpu_types.cuh (L69): hardcoded default size_t embedding_dim = 32;
config.hpp (L40): hardcoded token_dim = 1024;
config.hpp (L148): hardcoded in AxiomModelConfig: hidden_dim = 1536;
ai.cpp (L41): hardcoded embedding_dim = 1536;
ai.cpp (L68-70): embedding_dim = target_elements_y_total; if (embedding_dim == 0) embedding_dim = 4;
Why: If C++ allocates host buffers for
𝐷
=
1024
D=1024 or
𝐷
=
1536
D=1536 while CUDA's GpuEmbeddings defaults to
𝐷
=
32
D=32 or assumes
𝐷
=
4
D=4, pointer strides completely desynchronize, reading garbage memory or crashing with out-of-bounds VRAM access.

The Head Dimension Trap (head_dim > 64 vs 128 / 1024):

Where:
forward_pass.cu (L1317), backward_pass.cu (L786), paged_gqa.cu (L397): hardcoded if (head_dim == 0 || head_dim > 64) return false;
config.hpp (L151): hardcoded head_dim = 128;
test_ai.cpp (L589): calls build_transformer_block(out_dim, 1, 16) where out_dim = 1024, yielding head_dim = 1024 / 1 = 1024.
Why: The CUDA kernels strictly refuse any head dimension above 64 due to hardcoded shared-memory layout limits. Meanwhile, the C++ config specifies 128, and tests pass 1024. The CUDA kernel returns false silently, nothing executes, and uninitialized VRAM memory is returned as output.

Context Window & Sequence Length Desynchronization:

Where:
config.hpp (L41): context_window_size = 256;
config.hpp (L154): max_seq_len = 8192;
gpu_types.cuh (L30-31): hardcoded comments and struct sizing: (128 token IDs across PCIe)
test_ai.cpp (L593): hardcoded std::vector<std::bfloat16_t> xform_states(128 \* out_dim);
Why: Half the codebase assumes 128 tokens, the training config assumes 256 tokens, and the model config assumes 8,192 tokens. Changing the context window in C++ leaves CUDA kernels and test fixtures with misaligned buffer allocations.

Vocabulary Size Discrepancies:

Where:
config.hpp (L91): vocab::VOCAB_SIZE = 65536;
config.hpp (L153): AxiomModelConfig::vocab_size = 151936;
gpu_types.cuh (L68): size_t vocab_size = 65536;
Why: Two completely conflicting vocabulary sizes co-exist in the same configuration file without a single compile-time check or conversion rule.
B. Architectural Root Cause: The Disconnected Compilation Trees
The Split: None of the .cu files (forward_pass.cu, backward_pass.cu, paged_gqa.cu) include config.hpp.
Zero Static Asserts: There is not a single static_assert validating that CUDA buffer layouts match C++ struct layouts.
Scattered Magic Literals: Hardcoded literals like 256, 1024, 64, 32, 128, 4194304, 0.01f, 1e-8f, and 262144 are duplicated independently in over 20 different source and header files instead of referencing one shared definition.
