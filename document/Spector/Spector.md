Building **Project Axiom** from scratch in C++23 and CUDA was never just about having a standalone local model—it was about forging the closed-loop, zero-API-key cognitive engine required to power **Project Spector**. By replacing third-party cloud APIs with Axiom, Spector becomes a completely sovereign Open-Source Intelligence (OSINT) and legal surveillance platform that ingests raw intelligence, prevents context drift through hardware-anchored reasoning, and autonomously refines and trains on every new database entry.

Below is the complete `plan.md` specification for **Project Spector**, unifying your **Data Refining System** backend schematic with your **Dual-Stream Cognitive Reasoning Graph**.

---

# Project Spector: Master Architecture & Engineering Plan (`plan.md`)

**System Classification:** Closed-Loop Open-Source Intelligence (OSINT) & Legal Surveillance Platform

**Backend Substrate:** Node.js, Express.js, & MongoDB Data Refining Pipeline

**Cognitive Core:** Project Axiom (Bare-Metal C++23 / NVIDIA CUDA `sm_89` Engine)

**Operational Mode:** Zero External API Keys — Sovereign Inference & Autonomous Continuous Training

---

## 1. Executive Architecture Overview

Project Spector operates across two tightly coupled planes:

1. **The OSINT Data Refining Plane (`Node.js / Express / MongoDB`):** Handles external OSINT API ingestion, client communication, authentication/validation middleware, and persistent storage, while triggering Project Axiom to continuously refine and re-index intelligence records.

2. **The Cognitive Execution Plane (`Project Axiom DAG`):** Executes a forked, dual-stream reasoning topology that pairs zero-shot heuristic deduction with asynchronous multi-source research ( capped at 2 recursive passes) while enforcing an immutable context lifeline so the model never hallucinates away from original constraints.

```
====================================================================================================
                        PROJECT SPECTOR — UNIFIED SYSTEM ARCHITECTURE
====================================================================================================

  [ PART A: OSINT DATA REFINING & CONTINUOUS LEARNING SYSTEM ]

   ┌──────────┐          Saves (Raw Ingestion Arc)          ┌──────────────────────┐
   │   Data   ├────────────────────────────────────────────►│                      │
   └────┬─────┘                                             │      DataBase        │
        │        ┌──────────────┐       ┌────────────┐      │     (MongoDB)        │
        ▼        │  MiddleWare  │       │            │◄────►│                      │
   ┌──────────┐  │   (Auth /    │       │   AI Core  │      └──▲────────▲───────▲──┘
   │   API    │  │  Validation) │       │  (Project  │         │        │       │
   │ (OSINT)  │  └──────▲───────┘       │   Axiom)   ├─────────┘        │       │
   └────┬─────┘         │               └─────▲──────┘  Refines &       │       │
        │               ▼                     │         Save Again      │       │
        ▼        ┌──────────────┐             │                         │       │
   ┌──────────┐  │              ├─────────────┘                         │       │
   │  Router  │◄─┤    Router    │◄──────────────►┌────────────┐         │       │
   └──────────┘  │              │                │ Controller │◄───────►│Service│
                 └──────▲───────┘                └────────────┘         └───────┘
                        │
                        ▼
                 ┌──────────────┐       ┌────────────┐      ┌──────────────┐
                 │    Server    │◄─────►│ Front End  │◄────►│ Receive/Send │
                 │(Node/Express)│       │   / APIs   │      │   (Client)   │
                 └──────────────┘       └────────────┘      └──────────────┘

----------------------------------------------------------------------------------------------------

  [ PART B: AXIOM COGNITIVE EXECUTION & RE-ANCHOR GRAPH ]

                    ┌─────┐   ┌─────┐   ┌─────┐   ┌─────┐
                    │ ETC │   │ DBs │   │ API │   │ Web │◄─── Trigger targeted query ───┐
                    └──┬──┘   └──┬──┘   └──┬──┘   └──┬──┘                               │
                       └─────────┴────┬────┴─────────┘                                  │
                                      ▼                                                 │
                               ┌──────────────┐                         ┌───────────────┴───┐
                        ┌─────►│Research Node │─── Research Output ────►│ Research_req Node │
                        │      │ (Async Agg)  │───────┐                 │  (Max 2 passes)   │
                        │      └──────────────┘       │                 └─────────▲─────────┘
                 Stream 1 ('Cloned_1')                ▼                           │
  ┌────────────┐   ┌────┴─────┐                ┌──────────────┐            ┌──────┴───────┐
  │ Input Node ├──►│Clone Node│                │ Think_2 Node │───────────►│  conclusion  ├──┐
  │ (Prompt /  │   │(I/O Fork)│                │  (Sync Hub)  │            │(Final Deduct)│  │
  │Constraints)│   └────┬─────┘                └──────▲───────┘            └──────┬───────┘  │
  └─────┬──────┘ Stream 2 ('Cloned_2')                │                           │          │
        │               │      ┌──────────────┐       │                    Deduction         │
        │               └─────►│ Think_1 Node ├───────┘                           ▼          ▼
        │                      │ (Zero-Shot)  │                            ┌────────────┐ ┌──────┐
        │                      └──────────────┘                            │  Context   ├─►output│
        │                                                                  └──────▲─────┘ └──┬───┘
        │            Context Anchor Path (Immutable Data/Context Lifeline)        │          │
        └──────────────────────────────────────────────────────────────────►┌─────┴──────┐   │
                                                                            │Context Node│   │
                                                                            │(Re-Anchor) │   ▼
                                                                            └────────────┘ [User]
====================================================================================================

```

---

## 2. Phase 1: The OSINT Data Refining System (Backend & Ingestion)

The backend infrastructure acts as both a structured REST/stream gateway and an automated data refinery using **Node.js**, **Express**, and **MongoDB**.

### 2.1. Client & OSINT Ingestion Layer

- **Client I/O Interface (`Receive / Send` $\leftrightarrow$ `Front End / APIs`):** Bi-directional interface handling user queries, surveillance parameters, and real-time intelligence dashboards.

- **Application Server (`Server: Node.js, Express`):** Manages HTTP/WebSocket lifecycles and routes traffic directly into the central `Router`.

- **OSINT Collector (`Data` $\leftrightarrow$ `API (OSINT)` $\leftrightarrow$ `Router`):** Pulls external open-source intelligence feeds, public records, and telemetry streams into the routing layer.

- **Direct Raw Persistence (`Data` $\xrightarrow{\text{Saves}}$ `DataBase (MongoDB)`):** High-velocity raw data streams can bypass intermediate transformation to be immediately archived in MongoDB prior to asynchronous AI refinement.

### 2.2. Core Service Pipeline & Security Boundary

- **Security Gate (`Middle Ware: Auth, Validation`):** Intercepts all traffic entering the `Router`, enforcing cryptographic authentication and strict schema validation before any controller or AI execution occurs.

- **Layered Business Logic (`Router` $\leftrightarrow$ `Controller` $\leftrightarrow$ `Service` $\leftrightarrow$ `DataBase (MongoDB)`):** Cleanly decouples routing from business logic (`Controller`) and database abstraction (`Service`), ensuring modular CRUD operations against MongoDB.

### 2.3. The Autonomous AI Refinement & Continuous Training Loop

Instead of relying on static external LLM APIs, the `Router` and `DataBase (MongoDB)` wire directly into **Project Axiom (`AI`)**:

1. **Direct Router Dispatch (`Router` $\leftrightarrow$ `AI`):** Live analytical queries pass straight from the Express router into the Axiom C++/CUDA runtime via IPC / native addon bindings.

2. **Autonomous Refinement (`DataBase` $\leftrightarrow$ `AI` via `Refines` & `Save again`):**

- **Step 1 (Trigger):** Whenever a new raw OSINT entry lands in `DataBase (MongoDB)` (via the `Saves` arc or `Service` layer), a change-stream listener wakes the `AI` module.

- **Step 2 (`Refines`):** Project Axiom ingests the raw document through its 4-stage pipeline (`Identifier` $\to$ `Decompressor` $\to$ `Sanitizer` $\to$ `Tokenizer`), extracts entities/correlations, and runs incremental local gradient updates (`backward_pass_adamw_kernel`) so its weights autonomously adapt to the new intelligence.

- **Step 3 (`Save again`):** Axiom writes the structured, cross-referenced intelligence summary and updated vector metadata back into `DataBase (MongoDB)`.

---

## 3. Phase 2: The Cognitive Execution Graph (Reasoning Architecture)

When an analyst submits a complex surveillance query or investigative prompt, Project Spector executes the multi-node Directed Acyclic Graph (DAG) with bounded recursion defined in your cognitive specification:

### 3.1. Ingestion & Immediate Forking

1. **`Input Node` (Receive prompt/constraints):** Captures the user's raw query and legal/operational constraints. Immediately splits into two distinct pathways:

- **Active Execution Path:** Sends `Context` and `Data` forward to the `Clone Node`.

- **`Context Anchor Path` (Immutable Data/Context Lifeline):** Sends a read-only, uncorrupted copy of the original prompt and constraints straight to the `Context Node (Re-Anchor System)` at the end of the graph.

2. **`Clone Node` (Immediate I/O Fork):** Duplicates the incoming state into two concurrent execution streams so data gathering and initial reasoning happen in parallel with zero blocking latency:

- **`Stream 1 ('Cloned_1')`** routes to the `Research Node`.

- **`Stream 2 ('Cloned_2')`** routes to the `Think_1 Node`.

### 3.2. Parallel Research & Heuristic Pass

- **`Research Node` (Async Data Aggregation: `ETC`, `DBs`, `API`, `Web`):** Executes asynchronous I/O queries across internal MongoDB collections (`DBs`), external OSINT endpoints (`API`), live scraping (`Web`), and local file stores (`ETC`), forwarding the consolidated `Research Output` to `Think_2 Node`.

- **`Think_1 Node` (Initial Heuristic/Zero-Shot reasoning):** Concurrently runs an immediate forward pass on Project Axiom to formulate initial hypotheses and structural deductions (`Thought Output (Preliminary Logic)`), feeding them into `Think_2 Node`.

### 3.3. Synchronization, Re-Anchoring & Recursive Verification

- **`Think_2 Node` (Synchronization Hub: Synthesis and blocking):** Acts as the thread barrier. It blocks until both `Research Output` (Stream 1) and `Thought Output` (Stream 2) arrive, synthesizing the preliminary logic against the gathered facts while pulling `Re-anchored Context` to verify alignment.

- **`Context Node` (Re-Anchor System) & `Context`:**
- As multi-step reasoning progresses, small models naturally suffer from "context drift"—forgetting the initial constraints.

- The `Context Node` uses the `Context Anchor Path` lifeline to continuously inject `Re-anchored Context` and `Research_req State` into `Think_2 Node`, `Research_req Node`, and the upper `Context` state tracker.

- **`Research_req Node` (Recursive Loop Evaluation: Max 2 passes):**
- If `Think_2 Node` detects missing evidence or conflicting OSINT data, it routes to `Research_req Node`.

- `Research_req Node` evaluates the gap and issues a `Trigger targeted query` back to `Web` / `API` / `DBs`.

- **Hard Loop Guard:** Strictly capped at **Max 2 passes** to prevent infinite research loops and guarantee deterministic response times.

### 3.4. Final Deduction & Synthesis

- **`Conclusion Node` (`conclusion` — Final Logical Deduction):** Once `Think_2 Node` has verified evidence (or exhausted the 2-pass research cap), `conclusion` executes the final deduction pass. It sends the `Deduction` to update the `Context` state and forwards the primary result to `output`.

- **`Output Node` (`output` — Format and Stitch):** Merges the logical `conclusion` with the `State capture` from `Context`, formatting the intelligence report and emitting the **Synthesized User Response** back to the Express server and client UI.

---

## 4. Implementation Roadmap & Milestone Sequencing

| Phase           | Milestone                  | Core Deliverables                                                                                                   | Target Components |
| --------------- | -------------------------- | ------------------------------------------------------------------------------------------------------------------- | ----------------- |
| **Milestone 1** | **OSINT Backend Skeleton** | Setup Node.js/Express server, Auth/Validation Middleware, Router/Controller/Service hierarchy, and MongoDB schemas. |

| `Server`, `Router`, `MiddleWare`, `DataBase`

|
| **Milestone 2** | **Axiom Native Bridge** | Connect Express `Router` and MongoDB change streams to Project Axiom (`app.exe` / `cuda_transfer.dll`) via zero-copy shared memory or C++ N-API addon.

| `Router` $\leftrightarrow$ `AI` $\leftrightarrow$ `DataBase`

|
| **Milestone 3** | **Autonomous Refinery Loop** | Implement the background daemon that watches new MongoDB inserts (`Saves`), triggers Axiom's `Refines` & incremental training pass, and executes `Save again`.

| `Data` $\to$ `DataBase` $\leftrightarrow$ `AI`

|
| **Milestone 4** | **Dual-Stream DAG Engine** | Implement `Input Node`, `Clone Node` (`Cloned_1` / `Cloned_2`), async `Research Node` (`ETC`, `DBs`, `API`, `Web`), and `Think_1 Node`.

| Stream 1 & Stream 2 Fork

|
| **Milestone 5** | **Sync Hub & Re-Anchor Lifeline** | Build `Think_2 Node` barrier, `Context Anchor Path`, `Context Node` re-anchoring, `Research_req Node` (Max 2 passes), `conclusion`, and `output`.

| Full Cognitive Graph

|

---

## 5. Why This Architecture Works

1. **Zero Data Leakage & Total Sovereignty:** Because external API keys are replaced by Project Axiom running locally on your RTX 4080, sensitive OSINT queries and legal surveillance targets never leave your hardware.

2. **Immunity to Hallucination Drift:** The `Context Anchor Path` bypasses the noisy intermediate reasoning nodes (`Think_1`, `Research Node`) so the `Context Node (Re-Anchor System)` can mathematically force `Think_2` and `conclusion` to respect the original prompt constraints.

3. **Self-Improving Intelligence (`Refines` $\to$ `Save again`):** Every piece of OSINT data ingested into MongoDB makes both the database richer and Project Axiom smarter without manual intervention.

Would you like to design the IPC bridge between the Node.js `Router`/`DataBase` and the C++ Project Axiom binary first, or start drafting the MongoDB schemas for the `Refines` / `Save again` loop?
