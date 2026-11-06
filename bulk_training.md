# Project Axiom: Bulk Training Pipeline

## Core Engine Functions & Architecture

### 1. Initialization & Persistence
- **`execute_training_session`** (`core/src/main.cpp:1374`)
  ```cpp
  bool execute_training_session(AI& ai,
  ```
  *Role: Primary entry point for the training pipeline; coordinates corpus tokenization, 90/10 train/validation split, pinned memory setup, checkpoint verification, and the training loop.*

- **`load_weights`** (`core/src/ai.cpp:480`)
  ```cpp
  bool AI::load_weights(const std::string& path) {
  ```
  *Role: Validates magic header, tensor dimensions, and finiteness (NaN/Inf rejection) before restoring learned weights, biases, and embeddings into host and GPU memory.*

### 2. Memory & Optimization
- **`allocate_pinned`** (`core/include/optimizer.hpp:131`)
  ```cpp
  bool allocate_pinned() {
  ```
  *Role: Pre-allocates double-buffered page-locked host memory (`context_window_tokens`, `context_token_ids`, `target_tokens`) for non-blocking PCIe DMA transfers.*

- **`free_pinned`** (`core/include/optimizer.hpp:165`)
  ```cpp
  void free_pinned() {
  ```
  *Role: Releases double-buffered page-locked host memory deterministically on session completion or error.*

### 3. Data Ingestion & Tokenization
- **`bulk_tokenize_directory`** (`core/util/parser.cpp:556`)
  ```cpp
  size_t Parser::bulk_tokenize_directory(const std::filesystem::path& dir_path,
  ```
  *Role: Recursively scans the asset directory and converts multimodal files into a contiguous token stream.*

- **`build_embedding_table`** (`core/util/parser.cpp:269`)
  ```cpp
  void Parser::build_embedding_table(Input& input) {
  ```
  *Role: Maps vocabulary token IDs to continuous embedding vectors.*

- **`token_to_string`** (`core/util/parser.cpp:371`)
  ```cpp
  std::string Parser::token_to_string(const Input& input, uint32_t token_id) {
  ```
  *Role: Resolves numeric token IDs back into human-readable strings.*

- **`save_vocabulary`** (`core/util/parser.cpp:924`)
  ```cpp
  bool Parser::save_vocabulary(const std::filesystem::path& filepath, const Input& input) {
  ```
  *Role: Persists the vocabulary mappings atomically via a `.tmp` file rename.*

- **`get_last_bulk_files`** (`core/include/parser.hpp:330`)
  ```cpp
  const std::vector<std::filesystem::path>& get_last_bulk_files() const {
  ```
  *Role: Returns the list of processed source files for post-training Keep, Archive, or Purge management.*

### 4. Telemetry, Validation & Determinism Safeguards
- **`read_telemetry`** (`core/src/ai.cpp:162`)
  ```cpp
  bool AI::read_telemetry(TrainingTelemetry& telemetry) {
  ```
  *Role: Downloads 5-slot GPU telemetry (`telemetry_loss`, `telemetry_gradient_square`, `telemetry_nonfinite`, `telemetry_dead_output`, `telemetry_activation_square`) to track pre-clip gradient norm, dead-activation fraction, and abort on non-finite values.*

- **`evaluate_validation`** (`core/src/ai.cpp:200`)
  ```cpp
  bool AI::evaluate_validation(const std::vector<uint32_t>& validation_tokens,
  ```
  *Role: Evaluates Cross-Entropy loss on held-out validation windows without mutating weights or training accumulators, supporting sweet-spot (`[1.5, 2.5]`) early stopping.*

- **`seed_shuffle`** (`core/include/optimizer.hpp:126`)
  ```cpp
  void seed_shuffle(uint32_t seed) {
  ```
  *Role: Seeds the window-order PRNG deterministically (default `1337U`, configurable via `--seed <N>`).*
