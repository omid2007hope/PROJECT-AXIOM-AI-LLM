#include "../include/ai.hpp"
#include <cstring>
#include <algorithm>
#include <string>

static AI* g_engine = nullptr;

extern "C" {

__declspec(dllexport) bool axiom_load_model(const char* model_path, const char* vocab_path) {
    if (!g_engine) {
        g_engine = new AI();
    }
    if (vocab_path && std::strlen(vocab_path) > 0) {
        g_engine->load_vocabulary(vocab_path);
    }
    if (model_path && std::strlen(model_path) > 0) {
        return g_engine->load_axiom_model_mmap(model_path);
    }
    return true;
}

__declspec(dllexport) int axiom_generate_text(const char* prompt, char* out_buffer, int max_buffer_len, int max_new_tokens, float temperature) {
    if (!g_engine || !prompt || !out_buffer || max_buffer_len <= 0) return 0;
    std::string result;
    bool ok = g_engine->generate(prompt, max_new_tokens, temperature, 40, 0.9f, [&](const std::string& tok) {
        result += tok;
    });
    if (!ok) return 0;
    int copy_len = static_cast<int>(std::min(result.size(), static_cast<size_t>(max_buffer_len - 1)));
    std::memcpy(out_buffer, result.data(), copy_len);
    out_buffer[copy_len] = '\0';
    return copy_len;
}

__declspec(dllexport) void axiom_free_model() {
    if (g_engine) {
        delete g_engine;
        g_engine = nullptr;
    }
}

}

