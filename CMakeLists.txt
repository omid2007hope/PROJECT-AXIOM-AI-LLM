cmake_minimum_required(VERSION 3.22)
project(Axiom LANGUAGES CXX CUDA)

# ==============================================================================
# MANDATORY LICENSE ACCEPTANCE GATE (CLICKWRAP / COMMAND-WRAP ASSENT)
# ==============================================================================
option(AXIOM_ACCEPT_LICENSE "Affirmative acceptance of the Project Axiom Proprietary Source-Available License in ./LICENSE" OFF)

if(NOT AXIOM_ACCEPT_LICENSE)
    message(FATAL_ERROR
        "\n"
        "==============================================================================\n"
        "  PROPRIETARY LICENSE ACCEPTANCE REQUIRED (NOT OPEN SOURCE)\n"
        "==============================================================================\n"
        "  Project Axiom is proprietary, source-available software owned exclusively\n"
        "  by Omid Teimory. All Rights Reserved.\n\n"
        "  By compiling or building this software, you enter into a binding legal\n"
        "  agreement governed by the terms in the root ./LICENSE file.\n"
        "  Copying snippets, modifying, redistributing, commercial/SaaS use, and\n"
        "  AI/LLM training are strictly prohibited.\n\n"
        "  To affirmatively accept the terms of ./LICENSE and unlock compilation,\n"
        "  re-run CMake with the explicit assent flag:\n\n"
        "      cmake -B build -DAXIOM_ACCEPT_LICENSE=ON\n"
        "==============================================================================\n"
    )
else()
    message(STATUS "[LEGAL] Licensee has affirmatively accepted the Project Axiom Proprietary Source-Available License (./LICENSE).")
endif()

