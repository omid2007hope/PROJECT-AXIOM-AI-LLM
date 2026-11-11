// SPDX-License-Identifier: LicenseRef-Axiom-Proprietary
/*
 * Copyright (c) 2026 Omid Teimory. All Rights Reserved.
 *
 * PROPRIETARY SOURCE-AVAILABLE SOFTWARE — NOT OPEN SOURCE.
 * Permission to view, clone, or compile this file is granted strictly on the
 * condition precedent of full compliance with the root ./LICENSE file.
 * Copying snippets, modifying, creating derivative works, redistributing,
 * commercial/SaaS use, and AI/ML training (reserved under EU Directive
 * 2019/790 Art. 4(3) and § 42h(6) Austrian UrhG) are strictly prohibited.
 * Removal or alteration of this notice violates 17 U.S.C. § 1202 and § 90c UrhG.
 */

#ifndef CORE_INCLUDE_SHARED_PARAMS_H
#define CORE_INCLUDE_SHARED_PARAMS_H

#include <stddef.h>
#include <stdint.h>

// ─── CANONICAL SHARED HYPERPARAMETERS (C++ & CUDA UNIFIED SOURCE OF TRUTH) ──

// Canonical Vocabulary Capacity (V)
#define AXIOM_VOCAB_SIZE 65536U

// Canonical Token Representation Dimension (D)
#define AXIOM_TOKEN_DIM 1024U

// Canonical Context Window / Sequence Length (S)
#define AXIOM_CONTEXT_WINDOW 256U

// Multi-Head Attention Configuration
// Constraint: HEAD_DIM = TOKEN_DIM / NUM_HEADS = 1024 / 16 = 64 <= 64
#define AXIOM_NUM_HEADS 16U
#define AXIOM_HEAD_DIM 64U
#define AXIOM_MAX_HEAD_DIM 64U

// Feedforward SwiGLU Dimension (FFN = 4 * D)
#define AXIOM_FFN_DIM 4096U

// Total Flattened Sequence Fan-in Dimension (K = S * D = 262,144)
#define AXIOM_FAN_IN_DIM (AXIOM_CONTEXT_WINDOW * AXIOM_TOKEN_DIM)

#ifdef __cplusplus
namespace axiom {

constexpr uint32_t VOCAB_SIZE = AXIOM_VOCAB_SIZE;
constexpr size_t TOKEN_DIM = AXIOM_TOKEN_DIM;
constexpr size_t CONTEXT_WINDOW = AXIOM_CONTEXT_WINDOW;
constexpr size_t NUM_HEADS = AXIOM_NUM_HEADS;
constexpr size_t HEAD_DIM = AXIOM_HEAD_DIM;
constexpr size_t MAX_HEAD_DIM = AXIOM_MAX_HEAD_DIM;
constexpr size_t FFN_DIM = AXIOM_FFN_DIM;
constexpr size_t FAN_IN_DIM = AXIOM_FAN_IN_DIM;

static_assert(TOKEN_DIM % NUM_HEADS == 0, "TOKEN_DIM must be evenly divisible by NUM_HEADS");
static_assert(HEAD_DIM <= MAX_HEAD_DIM, "HEAD_DIM cannot exceed MAX_HEAD_DIM (64)");
static_assert(HEAD_DIM * NUM_HEADS == TOKEN_DIM, "HEAD_DIM * NUM_HEADS must equal TOKEN_DIM");
static_assert(FAN_IN_DIM == CONTEXT_WINDOW * TOKEN_DIM, "FAN_IN_DIM must equal CONTEXT_WINDOW * TOKEN_DIM");

} // namespace axiom
#endif

#endif // CORE_INCLUDE_SHARED_PARAMS_H

