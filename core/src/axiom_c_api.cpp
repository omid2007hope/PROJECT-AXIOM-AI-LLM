#include "../include/ai.hpp"
#include "../include/Identifier.hpp"
#include "../include/decompressor/decompress_document.hpp"
#include "../include/decompressor/decompress_audio.hpp"
#include <cstring>
#include <algorithm>
#include <sstream>
#include <string>

static AI* g_engine = nullptr;

extern "C" {

__declspec(dllexport) int axiom_extract_document(
    const char* filepath,
    char* out_buffer,
    int out_buffer_capacity
) {
    if (!filepath || !out_buffer || out_buffer_capacity <= 1) return -1;

    IdentifiedFile info = Identifier::identify(filepath);
    if (!info.is_valid || (!Identifier::is_document(info.format) && !Identifier::is_audio(info.format))) {
        return -2; // Not a supported document container
    }

    std::string extracted_utf8;
    if (Identifier::is_document(info.format)) {
        extracted_utf8 = DecompressDocument::extract_text(filepath);
    } else if (Identifier::is_audio(info.format)) {
        DecodedAudio audio = DecompressAudio::decode_audio(filepath);
        if (audio.is_valid) {
            std::ostringstream oss;
            double duration_sec = 0.0;
            int bytes_per_sample = (audio.bits_per_sample > 0 ? audio.bits_per_sample / 8 : 2) *
                                   (audio.channels > 0 ? audio.channels : 1);
            if (audio.sample_rate > 0 && bytes_per_sample > 0) {
                duration_sec = static_cast<double>(audio.pcm_bytes.size()) /
                               static_cast<double>(audio.sample_rate * bytes_per_sample);
            }
            oss << "[AXIOM AUDIO FORENSICS] Format: " << Identifier::format_to_string(info.format)
                << " | Sample Rate: " << audio.sample_rate << " Hz"
                << " | Channels: " << audio.channels
                << " | Bit Depth: " << audio.bits_per_sample << "-bit"
                << " | PCM Payload: " << audio.pcm_bytes.size() << " bytes"
                << " | Duration: " << duration_sec << "s";
            extracted_utf8 = oss.str();
        }
    }

    if (extracted_utf8.empty()) return 0;

    size_t copy_len = extracted_utf8.size();
    if (copy_len >= static_cast<size_t>(out_buffer_capacity)) {
        copy_len = static_cast<size_t>(out_buffer_capacity - 1);
    }
    std::memcpy(out_buffer, extracted_utf8.data(), copy_len);
    out_buffer[copy_len] = '\0';
    return static_cast<int>(copy_len);
}

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
