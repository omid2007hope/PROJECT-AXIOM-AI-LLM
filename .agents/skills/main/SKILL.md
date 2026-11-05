---
name: main
description: Primary architectural guide and development workflow for Project Axiom C++23 and CUDA engine, local transformer inference, server pipelines, and tests.
---

# Project Axiom Master Development Guide

Project Axiom is a low-latency C++23 and CUDA high-performance computing system featuring stacked GQA Transformer inference, Win32 DMA weight streaming, custom CUDA kernels, and a dual OpenAI/Ollama HTTP API.

## Project Structure

- `core/include/`: C++23 header definitions (`ai.hpp`, `server.hpp`, `cuda_transfer.hpp`, `parser.hpp`, `identifier.hpp`, `transfer.hpp`, `optimizer.hpp`).
- `core/src/`: Core implementation sources:
  - `server.cpp`: Dual OpenAI + Ollama HTTP server with native IDE tool calling (`editor`, `replace_in_file`).
  - `ai.cpp`: Stacked GQA Transformer forward pass, autoregressive token generator, and prefill management.
  - `main.cpp`: Interactive CLI console, telemetry HUD, test runners, and server controller.
- `core/src/CUDA/kernel/`: Custom CUDA kernels:
  - `paged_gqa.cu`: Dynamic shared-memory Paged Grouped Query Attention decoder.
  - `forward_pass.cu`: Fused GQA kernels, RoPE, RMSNorm, SwiGLU, and Cutlass GEMM.
- `models/`: Weights and tokenizers:
  - `models/qwen3b.axiom`: 3B parameter FP16 weights (~6.48 GB VRAM). Fits 100% in 12GB laptop GPUs.
  - `models/qwen7b.axiom`: 7B parameter BF16 weights (~15.2 GB).
  - `models/qwen_vocab.axiom`: 151,936-token vocabulary file.
- `.agents/skills/`: Workspace agent skills:
  - `axiom-server`: Dual server endpoints, lifecycle management, and IDE tool integration.

---

## Build & Test Commands

Build the system with MinGW GCC (C++23):
```powershell
cd "core"
Stop-Process -Name app -Force -ErrorAction SilentlyContinue
mingw32-make -j8 app.exe
```

Run test suite:
```powershell
cd "core"
.\app.exe --test
```

Start the dual server:
```powershell
cd "core"
.\app.exe --serve
```
