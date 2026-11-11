. Risks & Deferred
Deferred to Phase 2: Mathematical gradient derivation in compute_loss_and_dz_kernel, AdamW step-gain fan-in normalization in backward_pass_adamw_kernel, and missing inv_temp scaling.
Deferred to Phase 3: Forward pass structural mismatches (RMSNorm sequence-wide collapse E1 and flattened readout projection perceptron parameter explosion).
Deferred to Phase 4: Data ingestion cleaning (PDF high-byte ligature filter G1, token frequency underflow G2, and full context window supervision H2).
Full-Sequence Autoregressive Supervision (H2): Currently, only the final token
ℎ
𝑆
−
1
h
S−1
​

receives backpropagation supervision per window. Vectorized full-sequence cross-entropy (
𝑆
−
1
S−1 tokens supervised concurrently) is recommended for next-stage throughput scaling.
