# Strategic Technical Review and Scaling Roadmap: Native C++/CUDA AI Engine
(Archived Prior to 19 October 2026 Milestone Review)

> [!NOTE]
> **Historical Archive Notice:** This document preserves the legacy 11-phase roadmap formulated during early prototype development. 
> For active engineering, consult the single authoritative source of truth:
> - **Primary Engineering Plan:** [document/plan/UnifiedMasterEngineeringPlan.md](file:///c:/Users/Omid/X/Code/PROJECT-AXIOM/document/plan/UnifiedMasterEngineeringPlan.md)
> - **Active Horizons Directive:** [document/plan/plan.md](file:///c:/Users/Omid/X/Code/PROJECT-AXIOM/document/plan/plan.md)
> - **Master Checklist:** [.todo](file:///c:/Users/Omid/X/Code/PROJECT-AXIOM/.todo)

---

## Architectural Mapping to Unified Master Plan

| Legacy Phase | Legacy Title | Modern Unified Phase & Status |
| :--- | :--- | :--- |
| **Phase 1** | Ingestion & Modality Unification (4-Stage Pipeline) | **Unified Phase 0 [LOCKED - PASSED]** |
| **Phase 2** | Dense Contiguous Early-Fusion Indexing ( = 65,536$) | **Unified Phase 0 [LOCKED - PASSED]** |
| **Phase 3** | PCIe Zero-Copy Pipeline & Token ID Inversion | **Unified Phase 0 [LOCKED - PASSED]** |
| **Phase 4** | Learnable VRAM Embedding Matrix & Linear EVT Compute | **Unified Phase 0 [LOCKED - PASSED]** |
| **Phase 5** | Curing AI Amnesia (AdamW Optimizer State Persistence)| **Unified Phase 0 [LOCKED - PASSED]** |
| **Phase 6** | Output Codecs / Detokenizers (Compressor Mirrors)   | **Unified Phase 6 [PENDING]** (Text detokenizer completed) |
| **Phase 7** | Userspace Best-Fit with Coalescing (BFC) Allocator  | **Unified Phase 2 [NEXT]** |
| **Phase 8** | Autoregressive State Management & PagedAttention    | **Unified Phase 1 [COMPLETED]** (Streaming & Sampling); **Unified Phase 4 [PENDING]** (PagedAttention) |
| **Phase 9** | Microsecond Overhead Eradication with CUDA Graphs  | **Unified Phase 5 [PENDING]** |
| **Phase 10**| ATen-Style Declarative Dispatcher                   | **Unified Phase 5 [PENDING]** |
| **Phase 11**| Sparse Mixture of Experts (MoE) Architecture        | **Unified Phase 6 [PENDING]** |

---

## Historical Phase Breakdown (As Archived)

### Phase 1: Ingestion & Modality Unification (The Four-Stage Pipeline)
[COMPLETED & VERIFIED IN TEST SUITE]
- Identifier & Decompressors implemented.
- Unified text & numbers in main_dictionary.

### Phase 2: Dense Contiguous Early-Fusion Indexing ( = 65,536$)
[COMPLETED & VERIFIED IN TEST SUITE]
- Fixed 16-bit addressable range $[0, 65,535]$.

### Phase 3: PCIe Zero-Copy Pipeline & Token ID Inversion
[COMPLETED & VERIFIED IN TEST SUITE]
- Pinned cudaHostAllocWriteCombined memory.
- Double-buffering across dual non-blocking CUDA streams.

### Phase 4: Learnable VRAM Embedding Matrix & Hardware Compute
[COMPLETED & VERIFIED IN TEST SUITE]
- CUTLASS GEMM on Tensor Cores with Linear EVT.
- Branchless forward gather kernel.
- Backward scatter kernel with tomicAdd.

### Phase 5: Curing AI Amnesia (The AdamW Optimizer State)
[COMPLETED & VERIFIED IN TEST SUITE]
- Fused AdamW CUDA kernel.
- 56-byte binary persistence header with atomic swap.

### Phase 6: Output Codecs / Detokenizers (The Compressor)
[TEXT COMPLETE; AUDIO/VIDEO STUBS PRESENT]
- Transitioned to Unified Phase 6.

### Phase 7: Userspace Best-Fit with Coalescing (BFC) Caching Allocator
[CONSOLIDATED INTO UNIFIED PHASE 2 - NEXT]

### Phase 8: Autoregressive State Management via PagedAttention
[SPLIT INTO UNIFIED PHASE 1 (STREAMING/SAMPLING - COMPLETED) & UNIFIED PHASE 4 (PAGEDATTENTION)]

### Phase 9: Microsecond Overhead Eradication with CUDA Graphs
[CONSOLIDATED INTO UNIFIED PHASE 5]

### Phase 10: ATen-Style Declarative Dispatcher
[CONSOLIDATED INTO UNIFIED PHASE 5]

### Phase 11: Sparse Mixture of Experts (MoE) Architecture
[CONSOLIDATED INTO UNIFIED PHASE 6]
