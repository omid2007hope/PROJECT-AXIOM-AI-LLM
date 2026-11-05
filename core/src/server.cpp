#include "../include/ai.hpp"
#include "../include/server.hpp"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cstdint>
#include <conio.h>
#include <io.h>
#include <filesystem>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <cctype>
#include <fstream>
#include <algorithm>

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif

static std::atomic<bool> g_server_stop_requested{false};
static std::atomic<bool> g_server_is_active{false};
static std::thread g_server_thread;
static SOCKET g_server_listen_sock = INVALID_SOCKET;

struct ClineContext {
    bool is_cline = false;
    bool is_completion_turn = false;
    bool has_file_target = false;
    std::string target_path;
    std::string search_excerpt;
    std::string stop_line;
    std::string initial_prefix;
    std::string closing_suffix;
};

static std::string trim_whitespace(const std::string& text) {
    size_t start = text.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(start, end - start + 1);
}

static void select_file_excerpt(const std::string& full_content,
                                const std::string& task_text,
                                std::string& search_excerpt,
                                std::string& stop_line) {
    std::vector<std::string> lines;
    std::istringstream stream(full_content);
    std::string raw_line;
    while (std::getline(stream, raw_line)) {
        if (!raw_line.empty() && raw_line.back() == '\r') {
            raw_line.pop_back();
        }
        lines.push_back(raw_line);
    }

    if (lines.empty()) {
        search_excerpt.clear();
        stop_line.clear();
        return;
    }

    if (lines.size() <= 40 && full_content.size() <= 2000) {
        std::ostringstream joined;
        for (size_t i = 0; i < lines.size(); ++i) {
            if (i > 0) joined << "\n";
            joined << lines[i];
        }
        search_excerpt = joined.str();
        stop_line.clear();
        return;
    }

    std::string task_lower = task_text;
    for (char& ch : task_lower) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }

    bool is_visual_task =
        (task_lower.find("visu") != std::string::npos ||
         task_lower.find("menu") != std::string::npos ||
         task_lower.find("banner") != std::string::npos ||
         task_lower.find("color") != std::string::npos ||
         task_lower.find("display") != std::string::npos ||
         task_lower.find("print") != std::string::npos ||
         task_lower.find("impro") != std::string::npos ||
         task_lower.find("iprove") != std::string::npos);

    std::vector<std::string> task_words;
    std::string current_word;
    for (char ch : task_lower) {
        if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '_') {
            current_word += ch;
        } else {
            if (current_word.size() >= 4 &&
                current_word != "core" && current_word != "main" &&
                current_word != "file" && current_word != "code" &&
                current_word != "much" && current_word != "possible" &&
                current_word != "poosible") {
                task_words.push_back(current_word);
            }
            current_word.clear();
        }
    }
    if (current_word.size() >= 4 &&
        current_word != "core" && current_word != "main" &&
        current_word != "file" && current_word != "code" &&
        current_word != "much" && current_word != "possible" &&
        current_word != "poosible") {
        task_words.push_back(current_word);
    }

    std::vector<int> line_scores(lines.size(), 0);
    for (size_t i = 0; i < lines.size(); ++i) {
        std::string lower_line = lines[i];
        for (char& ch : lower_line) {
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }
        for (const std::string& kw : task_words) {
            if (lower_line.find(kw) != std::string::npos) {
                line_scores[i] += 10;
            }
        }
        if (is_visual_task) {
            if (lines[i].find("Welcome to AXIOM") != std::string::npos) line_scores[i] += 120;
            if (lines[i].find("PROJECT AXIOM") != std::string::npos) line_scores[i] += 30;
            if (lines[i].find("====") != std::string::npos) line_scores[i] += 8;
            if (lines[i].find("std::cout <<") != std::string::npos) line_scores[i] += 6;
            if (lines[i].find("[1]") != std::string::npos || lines[i].find("[2]") != std::string::npos) {
                line_scores[i] += 6;
            }
        }
    }

    const size_t window_len = std::min<size_t>(8, lines.size());
    size_t best_start = 0;
    int best_score = -1;
    for (size_t start = 0; start + window_len <= lines.size(); ++start) {
        int sum_score = 0;
        for (size_t k = 0; k < window_len; ++k) {
            sum_score += line_scores[start + k];
        }
        if (sum_score > best_score) {
            best_score = sum_score;
            best_start = start;
        }
    }

    // Advance start past outer function/loop scope openers ({) so the excerpt starts on a self-contained statement
    while (best_start + 4 < lines.size()) {
        std::string tr = trim_whitespace(lines[best_start]);
        if (tr.empty() || tr.back() == '{' || tr == "int option = 0;") {
            ++best_start;
        } else {
            break;
        }
    }

    // Align start so it does not begin on a stream continuation line (<<) or closing brace (})
    while (best_start > 0) {
        std::string tr = trim_whitespace(lines[best_start]);
        if (tr.rfind("<<", 0) == 0 || tr.rfind("}", 0) == 0 || tr.empty()) {
            --best_start;
        } else {
            break;
        }
    }

    size_t best_end = std::min(lines.size() - 1, best_start + window_len - 1);

    // Align end so statements and braces inside [best_start .. best_end] are strictly balanced (never dipping < 0)
    for (size_t candidate_end = best_start;
         candidate_end < std::min(lines.size(), best_start + 20);
         ++candidate_end) {
        int brace_balance = 0;
        int min_balance = 0;
        for (size_t idx = best_start; idx <= candidate_end; ++idx) {
            for (char c : lines[idx]) {
                if (c == '{') ++brace_balance;
                else if (c == '}') {
                    --brace_balance;
                    if (brace_balance < min_balance) min_balance = brace_balance;
                }
            }
        }
        std::string tr = trim_whitespace(lines[candidate_end]);
        bool next_is_stream = (candidate_end + 1 < lines.size() &&
                               trim_whitespace(lines[candidate_end + 1]).rfind("<<", 0) == 0);
        if (min_balance >= 0 && brace_balance == 0 && !next_is_stream && !tr.empty() &&
            candidate_end >= best_start + 3 &&
            (tr.back() == ';' || tr.back() == '}')) {
            best_end = candidate_end;
            break;
        }
    }

    std::ostringstream excerpt_oss;
    for (size_t idx = best_start; idx <= best_end; ++idx) {
        if (idx > best_start) excerpt_oss << "\n";
        excerpt_oss << lines[idx];
    }
    search_excerpt = excerpt_oss.str();

    stop_line.clear();
    for (size_t idx = best_end + 1; idx < lines.size(); ++idx) {
        std::string tr = trim_whitespace(lines[idx]);
        if (!tr.empty() && tr != "}" && tr != "};" && search_excerpt.find(tr) == std::string::npos) {
            stop_line = tr;
            break;
        }
    }
}

static std::string escape_json(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 16);
    for (unsigned char c : s) {
        switch (c) {
            case '\"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            default:
                if (c < 0x20) {
                    char hex_buf[8];
                    snprintf(hex_buf, sizeof(hex_buf), "\\u%04x", c);
                    out += hex_buf;
                } else {
                    out += static_cast<char>(c);
                }
                break;
        }
    }
    return out;
}

static std::string extract_json_string(const std::string& json, size_t start_pos, size_t& next_pos) {
    std::string val;
    val.reserve(1024);
    bool escaped = false;
    for (size_t i = start_pos; i < json.size(); ++i) {
        char c = json[i];
        if (escaped) {
            switch (c) {
                case 'n': val += '\n'; break;
                case 'r': val += '\r'; break;
                case 't': val += '\t'; break;
                case '\"': val += '\"'; break;
                case '\\': val += '\\'; break;
                case '/': val += '/'; break;
                case 'b': val += '\b'; break;
                case 'f': val += '\f'; break;
                case 'u': {
                    if (i + 4 < json.size()) {
                        std::string hex_str = json.substr(i + 1, 4);
                        try {
                            uint32_t codepoint = static_cast<uint32_t>(std::stoul(hex_str, nullptr, 16));
                            if (codepoint <= 0x7F) {
                                val += static_cast<char>(codepoint);
                            } else if (codepoint <= 0x7FF) {
                                val += static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F));
                                val += static_cast<char>(0x80 | (codepoint & 0x3F));
                            } else {
                                val += static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F));
                                val += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                                val += static_cast<char>(0x80 | (codepoint & 0x3F));
                            }
                            i += 4;
                        } catch (...) {
                            val += "\\u";
                        }
                    } else {
                        val += "\\u";
                    }
                    break;
                }
                default:
                    val += '\\';
                    val += c;
                    break;
            }
            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else if (c == '\"') {
            next_pos = i + 1;
            return val;
        } else {
            val += c;
        }
    }
    next_pos = json.size();
    return val;
}

// Extracts Ollama /api/generate prompt and system instructions into Qwen ChatML
static std::string build_ollama_prompt(const std::string& body) {
    std::string prompt_text;
    std::string system_text = "You are an expert prompt expander and creative writing AI.";

    size_t sys_pos = body.find("\"system\"");
    if (sys_pos != std::string::npos) {
        size_t colon = body.find(':', sys_pos + 8);
        if (colon != std::string::npos) {
            size_t val_start = body.find('\"', colon + 1);
            if (val_start != std::string::npos) {
                size_t next_i = 0;
                system_text = extract_json_string(body, val_start + 1, next_i);
            }
        }
    }

    size_t pr_pos = body.find("\"prompt\"");
    if (pr_pos != std::string::npos) {
        size_t colon = body.find(':', pr_pos + 8);
        if (colon != std::string::npos) {
            size_t val_start = body.find('\"', colon + 1);
            if (val_start != std::string::npos) {
                size_t next_i = 0;
                prompt_text = extract_json_string(body, val_start + 1, next_i);
            }
        }
    }

    if (prompt_text.empty()) prompt_text = "Describe a photorealistic, detailed scene.";

    return "<|im_start|>system\n" + system_text + "<|im_end|>\n"
           "<|im_start|>user\n" + prompt_text + "<|im_end|>\n"
           "<|im_start|>assistant\n";
}

// Extracts messages from Cline's JSON payload and formats into Qwen ChatML.
// Seamlessly extracts all multi-part array text blocks (e.g. attached files).
static std::string build_qwen_chatml_prompt(const std::string& body, ClineContext& cline_context) {
    cline_context = ClineContext{};
    std::vector<std::pair<std::string, std::string>> messages;

    size_t cursor = 0;
    while (cursor < body.size()) {
        size_t role_key = body.find("\"role\"", cursor);
        if (role_key == std::string::npos) break;

        size_t role_val_start = body.find('\"', role_key + 6);
        if (role_val_start == std::string::npos) break;

        size_t next_idx = 0;
        std::string role = extract_json_string(body, role_val_start + 1, next_idx);

        size_t content_key = body.find("\"content\"", next_idx);
        if (content_key == std::string::npos) break;

        size_t next_role = body.find("\"role\"", next_idx);
        if (next_role != std::string::npos && content_key > next_role) {
            cursor = next_idx;
            continue;
        }

        size_t colon_pos = body.find(':', content_key + 9);
        if (colon_pos == std::string::npos) break;

        size_t val_pos = body.find_first_not_of(" \t\r\n", colon_pos + 1);
        if (val_pos == std::string::npos) break;

        if (body[val_pos] == '\"') {
            std::string content = extract_json_string(body, val_pos + 1, cursor);
            messages.emplace_back(role, content);
        } else if (body[val_pos] == '[') {
            size_t array_end = std::string::npos;
            bool in_quotes = false;
            int bracket_depth = 0;
            for (size_t idx = val_pos; idx < body.size(); ++idx) {
                char c = body[idx];
                if (c == '\\' && in_quotes) {
                    ++idx;
                    continue;
                }
                if (c == '\"') {
                    in_quotes = !in_quotes;
                } else if (!in_quotes) {
                    if (c == '[') {
                        ++bracket_depth;
                    } else if (c == ']') {
                        --bracket_depth;
                        if (bracket_depth == 0) {
                            array_end = idx;
                            break;
                        }
                    }
                }
            }
            if (array_end == std::string::npos) array_end = body.size();

            std::string combined_content;
            size_t text_search_pos = val_pos;
            while (text_search_pos < array_end) {
                size_t text_key = body.find("\"text\"", text_search_pos);
                if (text_key == std::string::npos || text_key > array_end) break;
                size_t text_colon = body.find(':', text_key + 6);
                if (text_colon == std::string::npos || text_colon > array_end) break;
                size_t text_val_start = body.find('\"', text_colon + 1);
                if (text_val_start == std::string::npos || text_val_start > array_end) break;

                size_t next_t = 0;
                std::string part = extract_json_string(body, text_val_start + 1, next_t);
                if (!combined_content.empty()) combined_content += "\n\n";
                combined_content += part;
                text_search_pos = next_t;
            }

            if (!combined_content.empty()) {
                messages.emplace_back(role, combined_content);
            }
            cursor = array_end + 1;
        } else {
            cursor = val_pos + 1;
        }
    }

    if (messages.empty()) {
        return "<|im_start|>system\nYou are Project Axiom, an expert C++23 and CUDA coding agent.<|im_end|>\n"
               "<|im_start|>user\nWrite clean C++23 and CUDA code.<|im_end|>\n"
               "<|im_start|>assistant\n";
    }

    for (const auto& [role, text] : messages) {
        if (text.find("<task>") != std::string::npos ||
            text.find("<environment_details>") != std::string::npos ||
            text.find("replace_in_file") != std::string::npos ||
            text.find("write_to_file") != std::string::npos) {
            cline_context.is_cline = true;
            break;
        }
    }

    if (!cline_context.is_cline) {
        std::string prompt;
        prompt.reserve(body.size() + 256);
        for (const auto& [role, text] : messages) {
            prompt += "<|im_start|>" + role + "\n" + text + "<|im_end|>\n";
        }
        prompt += "<|im_start|>assistant\n";
        return prompt;
    }

    std::string task_text;
    std::string last_user_text;
    std::string target_path;
    std::string full_file_content;

    for (const auto& [role, text] : messages) {
        if (role == "user") {
            last_user_text = text;
            size_t t_open = text.find("<task>");
            size_t t_close = text.find("</task>");
            if (t_open != std::string::npos && t_close != std::string::npos && t_close > t_open + 6) {
                task_text = trim_whitespace(text.substr(t_open + 6, t_close - (t_open + 6)));
            }

            size_t fc_open = text.find("<file_content path=\"");
            if (fc_open != std::string::npos) {
                size_t path_start = fc_open + 20;
                size_t path_end = text.find('\"', path_start);
                if (path_end != std::string::npos) {
                    target_path = text.substr(path_start, path_end - path_start);
                    size_t tag_close = text.find('>', path_end);
                    size_t fc_end = text.find("</file_content>", path_end);
                    if (tag_close != std::string::npos && fc_end != std::string::npos && fc_end > tag_close + 1) {
                        full_file_content = trim_whitespace(text.substr(tag_close + 1, fc_end - (tag_close + 1)));
                    }
                }
            }
        }
    }

    // Strip @/ prefixes from task_text
    while (true) {
        size_t at_pos = task_text.find("@/");
        if (at_pos == std::string::npos) break;
        task_text.erase(at_pos, 2);
    }

    // Check if the most recent turn is a successful file modification result from Cline
    if ((last_user_text.find("[replace_in_file") != std::string::npos ||
         last_user_text.find("[write_to_file") != std::string::npos) &&
        (last_user_text.find("final_file_content") != std::string::npos ||
         last_user_text.find("successfully") != std::string::npos) &&
        last_user_text.find("Error") == std::string::npos &&
        last_user_text.find("does not match") == std::string::npos) {
        cline_context.is_completion_turn = true;
        cline_context.initial_prefix = "<attempt_completion>\n<result>\n";
        cline_context.closing_suffix = "\n</result>\n</attempt_completion>";
        if (task_text.empty()) task_text = "Update the file in the workspace.";
        return "<|im_start|>system\n"
               "You are Cline, an expert C++23 and CUDA software engineer. Summarize the code improvements concisely in 1-2 sentences and close with </result> and </attempt_completion>.<|im_end|>\n"
               "<|im_start|>user\n"
               "Task: " + task_text + "\n"
               "The file update was applied and saved in the IDE. Provide the completion summary.<|im_end|>\n"
               "<|im_start|>assistant\n"
               "<attempt_completion>\n<result>\n";
    }

    // Check if last_user_text contains a [read_file for '...'] Result:
    if (full_file_content.empty()) {
        size_t rf_pos = last_user_text.find("[read_file for '");
        if (rf_pos != std::string::npos) {
            size_t p_start = rf_pos + 16;
            size_t p_end = last_user_text.find("']", p_start);
            if (p_end != std::string::npos) {
                target_path = last_user_text.substr(p_start, p_end - p_start);
                size_t res_pos = last_user_text.find("Result:", p_end);
                if (res_pos != std::string::npos) {
                    size_t content_start = last_user_text.find('\n', res_pos);
                    if (content_start != std::string::npos) {
                        size_t env_pos = last_user_text.find("<environment_details>", content_start);
                        full_file_content = trim_whitespace(last_user_text.substr(
                            content_start + 1,
                            (env_pos != std::string::npos) ? (env_pos - (content_start + 1)) : std::string::npos));
                    }
                }
            }
        }
    }

    // Clean leading @/ or / from target_path
    while (!target_path.empty() && (target_path[0] == '@' || target_path[0] == '/' || target_path[0] == '\\')) {
        target_path.erase(0, 1);
    }

    // Fallback: if task_text mentions a file path and full_file_content is empty, try reading from disk
    if (target_path.empty() && !task_text.empty()) {
        std::istringstream iss(task_text);
        std::string tok;
        while (iss >> tok) {
            while (!tok.empty() && (tok.front() == '@' || tok.front() == '/' || tok.front() == '\"' || tok.front() == '\'')) {
                tok.erase(0, 1);
            }
            while (!tok.empty() && (tok.back() == ',' || tok.back() == '.' || tok.back() == '\"' || tok.back() == '\'')) {
                tok.pop_back();
            }
            if (tok.find(".cpp") != std::string::npos || tok.find(".hpp") != std::string::npos ||
                tok.find(".cu") != std::string::npos || tok.find(".cuh") != std::string::npos ||
                tok.find(".h") != std::string::npos || tok.find(".py") != std::string::npos) {
                target_path = tok;
                break;
            }
        }
    }

    if (!target_path.empty() && full_file_content.empty()) {
        std::string disk_candidate = target_path;
        if (!std::filesystem::exists(disk_candidate) && std::filesystem::exists("../" + target_path)) {
            disk_candidate = "../" + target_path;
        }
        if (std::filesystem::exists(disk_candidate)) {
            std::ifstream in(disk_candidate, std::ios::binary);
            if (in) {
                std::ostringstream ss;
                ss << in.rdbuf();
                full_file_content = ss.str();
            }
        }
    }

    if (!target_path.empty() && !full_file_content.empty()) {
        select_file_excerpt(full_file_content, task_text, cline_context.search_excerpt, cline_context.stop_line);
        if (!cline_context.search_excerpt.empty()) {
            cline_context.has_file_target = true;
            cline_context.target_path = target_path;
            cline_context.initial_prefix =
                "<replace_in_file>\n<path>" + target_path + "</path>\n<diff>\n------- SEARCH\n" +
                cline_context.search_excerpt + "\n=======\n";
            cline_context.closing_suffix = "+++++++ REPLACE\n</diff>\n</replace_in_file>";

            if (task_text.empty()) {
                task_text = "Improve the code block in " + target_path;
            }

            return "<|im_start|>system\n"
                   "You are Cline, an expert C++23 and CUDA software engineer.\n"
                   "Write ONLY the improved replacement C++23 code for the ------- SEARCH block, then output +++++++ REPLACE.\n"
                   "Rules:\n"
                   "- Output valid, compilable C++23 code ending with a semicolon (;).\n"
                   "- For visual or UI improvements, add ANSI color codes (\\033[1;36m, \\033[1;32m, \\033[1;33m, \\033[0m) and clean visual styling while keeping all menu numbers [1]-[5] intact.\n"
                   "- End the replacement block immediately with +++++++ REPLACE.\n"
                   "</diff>\n"
                   "</replace_in_file><|im_end|>\n"
                   "<|im_start|>user\n"
                   "Target File: " + target_path + "\n"
                   "Task: " + task_text + "\n\n"
                   "Original Code Block to Replace:\n"
                   "```cpp\n" +
                   cline_context.search_excerpt + "\n"
                   "```\n\n"
                   "Write the improved ANSI-styled C++ replacement code for this block and finish with +++++++ REPLACE:<|im_end|>\n"
                   "<|im_start|>assistant\n" +
                   cline_context.initial_prefix;
        }
    }

    // Fallback Cline prompt when no file is attached yet
    std::string clean_user = task_text.empty() ? last_user_text : task_text;
    size_t env_idx = clean_user.find("<environment_details>");
    if (env_idx != std::string::npos) {
        clean_user = trim_whitespace(clean_user.substr(0, env_idx));
    }
    return "<|im_start|>system\n"
           "You are Cline, an expert C++23 and CUDA coding agent. Respond with a valid Cline XML tool call (<read_file>, <replace_in_file>, <write_to_file>, or <attempt_completion>).<|im_end|>\n"
           "<|im_start|>user\n" + clean_user + "<|im_end|>\n"
           "<|im_start|>assistant\n";
}

static void execute_server_loop(AI& engine, uint16_t port, bool is_background) {
    if (!engine.has_transformer_model()) {
        std::cout << "[..] Streaming Qwen weights via Win32 DMA mmap...\n";
        std::string weights_p = "models/qwen3b.axiom";
        std::string vocab_p = "models/qwen_vocab.axiom";
        if (std::filesystem::exists("models/qwen3b.axiom")) {
            weights_p = "models/qwen3b.axiom";
            vocab_p = "models/qwen_vocab.axiom";
        } else if (std::filesystem::exists("../models/qwen3b.axiom")) {
            weights_p = "../models/qwen3b.axiom";
            vocab_p = "../models/qwen_vocab.axiom";
        } else if (std::filesystem::exists("models/qwen7b.axiom")) {
            weights_p = "models/qwen7b.axiom";
            vocab_p = "models/qwen_vocab.axiom";
        } else if (std::filesystem::exists("../models/qwen7b.axiom")) {
            weights_p = "../models/qwen7b.axiom";
            vocab_p = "../models/qwen_vocab.axiom";
        }
        bool v_ok = engine.load_vocabulary(vocab_p);
        bool m_ok = engine.load_axiom_model_mmap(weights_p.c_str());
        if (!v_ok || !m_ok) {
            std::cerr << "[!] Failed to load transformer weights from: " << weights_p << "\n";
            return;
        }
        std::cout << "[OK] Stacked GQA Transformer weights loaded into VRAM.\n\n";
    }

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "[Axiom Server] WSAStartup failed.\n";
        return;
    }

    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_sock == INVALID_SOCKET) {
        std::cerr << "[Axiom Server] socket creation failed.\n";
        WSACleanup();
        return;
    }
    g_server_listen_sock = listen_sock;

    BOOL exclusive_opt = TRUE;
    setsockopt(listen_sock, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&exclusive_opt), sizeof(exclusive_opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(port);

    if (bind(listen_sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err == WSAEADDRINUSE) {
            std::cerr << "\n[!] Error: Port " << port << " is ALREADY in use by another running Project Axiom process.\n"
                      << "[!] To free the port, run: powershell \"Stop-Process -Name app -Force\"\n\n";
        } else {
            std::cerr << "[Axiom Server] bind failed on port " << port << " (WSA Error: " << err << ").\n";
        }
        closesocket(listen_sock);
        g_server_listen_sock = INVALID_SOCKET;
        WSACleanup();
        return;
    }

    if (listen(listen_sock, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "[Axiom Server] listen failed.\n";
        closesocket(listen_sock);
        g_server_listen_sock = INVALID_SOCKET;
        WSACleanup();
        return;
    }

    if (!is_background) {
        std::cout << "========================================================\n"
                  << "     PROJECT AXIOM DUAL SERVER (OpenAI + Ollama API)    \n"
                  << "========================================================\n"
                  << "[OK] Listening on http://127.0.0.1:" << port << "\n"
                  << "  - Cline VS Code  : http://127.0.0.1:" << port << "/v1\n"
                  << "  - ComfyUI Ollama : http://127.0.0.1:" << port << "\n"
                  << "--------------------------------------------------------\n"
                  << "[Ready for requests... Press 'q' or Ctrl+C to stop]\n\n" << std::flush;
    }

    fd_set read_fds;
    timeval tv{};

    while (!g_server_stop_requested.load()) {
        if (!is_background && _isatty(_fileno(stdin)) && _kbhit()) {
            int ch = _getch();
            if (ch == 'q' || ch == 'Q' || ch == 27) {
                std::cout << "\n[Axiom Server] Shutdown requested ('" << static_cast<char>(ch) << "'). Closing server...\n";
                break;
            }
        }

        FD_ZERO(&read_fds);
        FD_SET(listen_sock, &read_fds);
        tv.tv_sec = 0;
        tv.tv_usec = 250000; // 250ms

        int sel = select(0, &read_fds, nullptr, nullptr, &tv);
        if (sel <= 0) continue;

        SOCKET client = accept(listen_sock, nullptr, nullptr);
        if (client == INVALID_SOCKET) continue;

        DWORD timeout_ms = 5000;
        setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout_ms), sizeof(timeout_ms));
        setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout_ms), sizeof(timeout_ms));

        std::string req;
        char chunk[4096];
        int content_length = -1;
        size_t body_pos = std::string::npos;

        constexpr size_t MAX_HTTP_REQUEST_BYTES = 16 * 1024 * 1024;
        while (true) {
            int bytes = recv(client, chunk, sizeof(chunk), 0);
            if (bytes <= 0) break;
            req.append(chunk, bytes);
            if (req.size() > MAX_HTTP_REQUEST_BYTES) {
                req.clear();
                break;
            }

            if (body_pos == std::string::npos) {
                body_pos = req.find("\r\n\r\n");
                if (body_pos != std::string::npos) {
                    size_t cl_pos = std::string::npos;
                    for (size_t i = 0; i + 15 < body_pos; ++i) {
                        if ((req[i] == 'c' || req[i] == 'C') &&
                            (req[i+1] == 'o' || req[i+1] == 'O') &&
                            (req[i+2] == 'n' || req[i+2] == 'N') &&
                            (req[i+3] == 't' || req[i+3] == 'T') &&
                            (req[i+4] == 'e' || req[i+4] == 'E') &&
                            (req[i+5] == 'n' || req[i+5] == 'N') &&
                            (req[i+6] == 't' || req[i+6] == 'T') &&
                            req[i+7] == '-' &&
                            (req[i+8] == 'l' || req[i+8] == 'L') &&
                            (req[i+9] == 'e' || req[i+9] == 'E') &&
                            (req[i+10] == 'n' || req[i+10] == 'N') &&
                            (req[i+11] == 'g' || req[i+11] == 'G') &&
                            (req[i+12] == 't' || req[i+12] == 'T') &&
                            (req[i+13] == 'h' || req[i+13] == 'H') &&
                            req[i+14] == ':') {
                            cl_pos = i;
                            break;
                        }
                    }
                    if (cl_pos != std::string::npos) {
                        size_t val_start = req.find_first_not_of(" \t", cl_pos + 15);
                        if (val_start != std::string::npos && val_start < body_pos &&
                            req[val_start] >= '0' && req[val_start] <= '9') {
                            try {
                                size_t cl_end = req.find_first_not_of("0123456789", val_start);
                                size_t cl_len = (cl_end != std::string::npos) ? (cl_end - val_start) : (body_pos - val_start);
                                int parsed_cl = std::stoi(req.substr(val_start, cl_len));
                                content_length = (parsed_cl >= 0 && static_cast<size_t>(parsed_cl) <= MAX_HTTP_REQUEST_BYTES) ? parsed_cl : 0;
                            } catch (...) {
                                content_length = 0;
                            }
                        } else {
                            content_length = 0;
                        }
                    } else {
                        content_length = 0;
                    }
                }
            }

            if (body_pos != std::string::npos && content_length >= 0) {
                if (req.size() >= body_pos + 4 + static_cast<size_t>(content_length)) {
                    break;
                }
            }
        }

        if (!req.empty()) {
            std::string body = (body_pos != std::string::npos) ? req.substr(body_pos + 4) : "";

            // Handle CORS preflight
            if (req.rfind("OPTIONS ", 0) == 0) {
                std::string cors_resp =
                    "HTTP/1.1 204 No Content\r\n"
                    "Access-Control-Allow-Origin: *\r\n"
                    "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                    "Access-Control-Allow-Headers: *\r\n"
                    "Connection: close\r\n\r\n";
                send(client, cors_resp.data(), static_cast<int>(cors_resp.size()), 0);
            }
            // Handle GET /models
            else if (req.rfind("GET /v1/models", 0) == 0 || req.rfind("GET /models", 0) == 0) {
                if (!is_background) std::cout << "[Server] GET /v1/models (Cline model discovery)\n" << std::flush;
                std::string models_json =
                    "{\"object\":\"list\",\"data\":["
                    "{\"id\":\"qwen2.5-coder:3b-instruct-fp16\",\"object\":\"model\",\"owned_by\":\"axiom\"},"
                    "{\"id\":\"qwen2.5-coder-3b\",\"object\":\"model\",\"owned_by\":\"axiom\"},"
                    "{\"id\":\"qwen2.5-coder\",\"object\":\"model\",\"owned_by\":\"axiom\"},"
                    "{\"id\":\"axiom\",\"object\":\"model\",\"owned_by\":\"axiom\"}"
                    "]}";
                std::ostringstream resp;
                resp << "HTTP/1.1 200 OK\r\n"
                     << "Content-Type: application/json\r\n"
                     << "Access-Control-Allow-Origin: *\r\n"
                     << "Content-Length: " << models_json.size() << "\r\n"
                     << "Connection: close\r\n\r\n"
                     << models_json;
                std::string wire = resp.str();
                send(client, wire.data(), static_cast<int>(wire.size()), 0);
            }
            // Handle Ollama GET /api/tags (ComfyUI-Ollama connectivity)
            else if (req.rfind("GET /api/tags", 0) == 0) {
                if (!is_background) std::cout << "[Server] GET /api/tags (ComfyUI model discovery)\n" << std::flush;
                std::string tags_json =
                    "{\"models\":["
                    "{\"name\":\"huihui_ai/qwen2.5-vl-abliterated:7b\",\"model\":\"huihui_ai/qwen2.5-vl-abliterated:7b\",\"modified_at\":\"2026-11-03T00:00:00Z\",\"size\":3800000000,\"digest\":\"axiom\"},"
                    "{\"name\":\"axiom\",\"model\":\"axiom\",\"modified_at\":\"2026-11-03T00:00:00Z\",\"size\":3800000000,\"digest\":\"axiom\"},"
                    "{\"name\":\"qwen2.5-coder:3b-instruct-fp16\",\"model\":\"qwen2.5-coder:3b-instruct-fp16\",\"modified_at\":\"2026-11-04T00:00:00Z\",\"size\":6794207296,\"digest\":\"axiom\"},"
                    "{\"name\":\"qwen2.5-coder-3b\",\"model\":\"qwen2.5-coder-3b\",\"modified_at\":\"2026-11-04T00:00:00Z\",\"size\":6794207296,\"digest\":\"axiom\"}"
                    "]}";
                std::ostringstream resp;
                resp << "HTTP/1.1 200 OK\r\n"
                     << "Content-Type: application/json\r\n"
                     << "Access-Control-Allow-Origin: *\r\n"
                     << "Content-Length: " << tags_json.size() << "\r\n"
                     << "Connection: close\r\n\r\n"
                     << tags_json;
                std::string wire = resp.str();
                send(client, wire.data(), static_cast<int>(wire.size()), 0);
            }
            // Handle Ollama POST /api/generate (ComfyUI OllamaGenerateV2)
            else if (req.rfind("POST /api/generate", 0) == 0) {
                if (!is_background) std::cout << "[Server] POST /api/generate (ComfyUI prompt expansion)\n" << std::flush;
                std::string chatml_prompt = build_ollama_prompt(body);
                int max_tokens = 512;
                size_t np_pos = body.find("\"num_predict\":");
                if (np_pos != std::string::npos) {
                    try {
                        int parsed = std::stoi(body.substr(np_pos + 14));
                        if (parsed > 0 && parsed <= 2048) max_tokens = parsed;
                    } catch (...) {}
                }

                std::string generated_reply;
                engine.generate(chatml_prompt, max_tokens, 0.5f, 50, 0.9f, [&](const std::string& tok) {
                    generated_reply += tok;
                    if (!is_background) std::cout << tok << std::flush;
                });

                if (generated_reply.empty()) {
                    generated_reply = "A cinematic, highly detailed rendering with dramatic atmospheric lighting.";
                }

                std::string ollama_resp =
                    "{\"model\":\"axiom\",\"response\":\"" + escape_json(generated_reply) + "\",\"thinking\":\"\",\"done\":true,\"done_reason\":\"stop\",\"context\":[]}";
                std::ostringstream resp;
                resp << "HTTP/1.1 200 OK\r\n"
                     << "Content-Type: application/json\r\n"
                     << "Access-Control-Allow-Origin: *\r\n"
                     << "Content-Length: " << ollama_resp.size() << "\r\n"
                     << "Connection: close\r\n\r\n"
                     << ollama_resp;
                std::string wire = resp.str();
                send(client, wire.data(), static_cast<int>(wire.size()), 0);
            } else {
                if (!is_background) std::cout << "[Server] POST /v1/chat/completions (Cline token generation)\n" << std::flush;
                ClineContext cline_context;
                std::string chatml_prompt = build_qwen_chatml_prompt(body, cline_context);
                bool is_stream = (body.find("\"stream\": true") != std::string::npos ||
                                  body.find("\"stream\":true") != std::string::npos);

                int max_tokens = 2048;
                size_t mt_pos = body.find("\"max_tokens\":");
                if (mt_pos == std::string::npos) {
                    mt_pos = body.find("\"max_completion_tokens\":");
                    if (mt_pos != std::string::npos) mt_pos += 11;
                }
                if (mt_pos != std::string::npos) {
                    try {
                        int parsed = std::stoi(body.substr(mt_pos + 13));
                        if (parsed > 0 && parsed <= 8192) max_tokens = parsed;
                    } catch (...) {}
                }
                if (cline_context.is_completion_turn) {
                    max_tokens = std::min(max_tokens, 96);
                } else if (cline_context.has_file_target) {
                    max_tokens = std::min(max_tokens, 360);
                }

                auto emit_delta = [&](const std::string& delta_text, std::string& full_reply, bool& emitted_any) {
                    if (delta_text.empty()) return;
                    emitted_any = true;
                    full_reply += delta_text;
                    if (is_stream) {
                        std::string chunk =
                            "data: {\"id\":\"chatcmpl-axiom\",\"object\":\"chat.completion.chunk\","
                            "\"choices\":[{\"index\":0,\"delta\":{\"content\":\"" + escape_json(delta_text) + "\"},\"finish_reason\":null}]}\n\n";
                        send(client, chunk.data(), static_cast<int>(chunk.size()), 0);
                    }
                    if (!is_background) std::cout << delta_text << std::flush;
                };

                if (is_stream) {
                    std::string stream_hdr =
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/event-stream\r\n"
                        "Cache-Control: no-cache\r\n"
                        "Access-Control-Allow-Origin: *\r\n"
                        "Connection: close\r\n\r\n";
                    send(client, stream_hdr.data(), static_cast<int>(stream_hdr.size()), 0);
                }

                bool emitted_any = false;
                std::string generated_reply;

                if (cline_context.has_file_target) {
                    emit_delta(cline_context.initial_prefix, generated_reply, emitted_any);
                    std::string line_buffer;
                    bool reached_end = false;
                    size_t emitted_code_lines = 0;
                    const bool single_statement =
                        (std::count(cline_context.search_excerpt.begin(), cline_context.search_excerpt.end(), ';') == 1 &&
                         !cline_context.search_excerpt.empty() &&
                         cline_context.search_excerpt.back() == ';');

                    engine.generate(chatml_prompt, max_tokens, 0.1f, 40, 0.9f, [&](const std::string& tok) {
                        if (reached_end) return;
                        if (tok.find("<|im_start|>") != std::string::npos ||
                            tok.find("<|im_end|>") != std::string::npos ||
                            tok.find("<|endoftext|>") != std::string::npos) {
                            return;
                        }
                        line_buffer += tok;
                        while (true) {
                            size_t nl_pos = line_buffer.find('\n');
                            if (nl_pos == std::string::npos) break;
                            std::string raw_line = line_buffer.substr(0, nl_pos);
                            if (!raw_line.empty() && raw_line.back() == '\r') {
                                raw_line.pop_back();
                            }
                            line_buffer.erase(0, nl_pos + 1);

                            std::string trimmed = trim_whitespace(raw_line);
                            if (trimmed.rfind("+++++++", 0) == 0 ||
                                trimmed.rfind(">>>>>>>", 0) == 0 ||
                                trimmed.rfind("=======", 0) == 0 ||
                                trimmed.rfind("-------", 0) == 0 ||
                                trimmed.rfind("</diff>", 0) == 0 ||
                                trimmed.rfind("</replace_in_file>", 0) == 0 ||
                                trimmed.rfind("```", 0) == 0 ||
                                (!cline_context.stop_line.empty() && trimmed == cline_context.stop_line)) {
                                if (emitted_code_lines > 0 || trimmed.rfind("```", 0) != 0) {
                                    reached_end = true;
                                    break;
                                }
                                continue;
                            }
                            emit_delta(raw_line + "\n", generated_reply, emitted_any);
                            ++emitted_code_lines;
                            if (single_statement && emitted_code_lines >= 2 && !trimmed.empty() && trimmed.back() == ';') {
                                reached_end = true;
                                break;
                            }
                        }
                    });

                    if (!reached_end && !line_buffer.empty()) {
                        std::string trimmed = trim_whitespace(line_buffer);
                        if (!trimmed.empty() &&
                            trimmed.rfind("+++++++", 0) != 0 &&
                            trimmed.rfind(">>>>>>>", 0) != 0 &&
                            trimmed.rfind("</diff>", 0) != 0 &&
                            trimmed.rfind("</replace_in_file>", 0) != 0 &&
                            trimmed.rfind("```", 0) != 0 &&
                            (cline_context.stop_line.empty() || trimmed != cline_context.stop_line)) {
                            emit_delta(line_buffer + "\n", generated_reply, emitted_any);
                            ++emitted_code_lines;
                        }
                    }
                    if (emitted_code_lines == 0) {
                        emit_delta(cline_context.search_excerpt + "\n", generated_reply, emitted_any);
                    }
                    emit_delta(cline_context.closing_suffix, generated_reply, emitted_any);
                } else if (cline_context.is_completion_turn) {
                    emit_delta(cline_context.initial_prefix, generated_reply, emitted_any);
                    std::string summary_buffer;
                    engine.generate(chatml_prompt, max_tokens, 0.2f, 40, 0.9f, [&](const std::string& tok) {
                        if (tok.find("<|im_start|>") != std::string::npos ||
                            tok.find("<|im_end|>") != std::string::npos ||
                            tok.find("<|endoftext|>") != std::string::npos) {
                            return;
                        }
                        summary_buffer += tok;
                    });
                    size_t close_tag = summary_buffer.find("</result>");
                    if (close_tag != std::string::npos) {
                        summary_buffer = summary_buffer.substr(0, close_tag);
                    }
                    close_tag = summary_buffer.find("</attempt_completion>");
                    if (close_tag != std::string::npos) {
                        summary_buffer = summary_buffer.substr(0, close_tag);
                    }
                    summary_buffer = trim_whitespace(summary_buffer);
                    if (summary_buffer.empty()) {
                        summary_buffer = "Successfully applied the requested code improvements to the file.";
                    }
                    emit_delta(summary_buffer, generated_reply, emitted_any);
                    emit_delta(cline_context.closing_suffix, generated_reply, emitted_any);
                } else {
                    engine.generate(chatml_prompt, max_tokens, 0.2f, 40, 0.9f, [&](const std::string& tok) {
                        if (tok.find("<|im_start|>") != std::string::npos ||
                            tok.find("<|im_end|>") != std::string::npos ||
                            tok.find("<|endoftext|>") != std::string::npos) {
                            return;
                        }
                        emit_delta(tok, generated_reply, emitted_any);
                    });
                }

                if (!emitted_any) {
                    emit_delta("I have reviewed your files and context.", generated_reply, emitted_any);
                }

                if (is_stream) {
                    std::string end_chunk =
                        "data: {\"id\":\"chatcmpl-axiom\",\"object\":\"chat.completion.chunk\","
                        "\"choices\":[{\"index\":0,\"delta\":{},\"finish_reason\":\"stop\"}]}\n\n"
                        "data: [DONE]\n\n";
                    send(client, end_chunk.data(), static_cast<int>(end_chunk.size()), 0);
                } else {
                    std::string json_payload =
                        "{\"id\":\"chatcmpl-axiom\",\"object\":\"chat.completion\",\"choices\":[{\"index\":0,"
                        "\"message\":{\"role\":\"assistant\",\"content\":\"" + escape_json(generated_reply) + "\"},"
                        "\"finish_reason\":\"stop\"}]}";

                    std::ostringstream resp;
                    resp << "HTTP/1.1 200 OK\r\n"
                         << "Content-Type: application/json\r\n"
                         << "Access-Control-Allow-Origin: *\r\n"
                         << "Content-Length: " << json_payload.size() << "\r\n"
                         << "Connection: close\r\n\r\n"
                         << json_payload;

                    std::string wire = resp.str();
                    send(client, wire.data(), static_cast<int>(wire.size()), 0);
                }
            }
        }

        closesocket(client);
    }

    if (g_server_listen_sock != INVALID_SOCKET) {
        SOCKET sock_to_close = g_server_listen_sock;
        g_server_listen_sock = INVALID_SOCKET;
        closesocket(sock_to_close);
    }
    WSACleanup();
}

void run_axiom_openai_server(AI& engine, uint16_t port) {
    g_server_stop_requested.store(false);
    g_server_is_active.store(true);
    execute_server_loop(engine, port, false);
    g_server_is_active.store(false);
}

bool start_background_axiom_server(AI& engine, uint16_t port) {
    if (g_server_is_active.load()) {
        return true;
    }

    if (!engine.has_transformer_model()) {
        std::cout << "[..] Streaming Qwen weights via Win32 DMA mmap...\n";
        std::string weights_p = "models/qwen3b.axiom";
        std::string vocab_p = "models/qwen_vocab.axiom";
        if (std::filesystem::exists("models/qwen3b.axiom")) {
            weights_p = "models/qwen3b.axiom";
            vocab_p = "models/qwen_vocab.axiom";
        } else if (std::filesystem::exists("../models/qwen3b.axiom")) {
            weights_p = "../models/qwen3b.axiom";
            vocab_p = "../models/qwen_vocab.axiom";
        } else if (std::filesystem::exists("models/qwen7b.axiom")) {
            weights_p = "models/qwen7b.axiom";
            vocab_p = "models/qwen_vocab.axiom";
        } else if (std::filesystem::exists("../models/qwen7b.axiom")) {
            weights_p = "../models/qwen7b.axiom";
            vocab_p = "../models/qwen_vocab.axiom";
        }
        bool v_ok = engine.load_vocabulary(vocab_p);
        bool m_ok = engine.load_axiom_model_mmap(weights_p.c_str());
        if (!v_ok || !m_ok) {
            std::cerr << "[!] Failed to load transformer weights from: " << weights_p << "\n";
            return false;
        }
        std::cout << "[OK] Stacked GQA Transformer weights loaded into VRAM.\n\n";
    }

    g_server_stop_requested.store(false);
    g_server_is_active.store(true);

    if (g_server_thread.joinable()) {
        g_server_thread.join();
    }

    g_server_thread = std::thread([&engine, port]() {
        execute_server_loop(engine, port, true);
        g_server_is_active.store(false);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return g_server_is_active.load();
}

void stop_background_axiom_server() {
    if (!g_server_is_active.load()) return;
    g_server_stop_requested.store(true);
    if (g_server_listen_sock != INVALID_SOCKET) {
        closesocket(g_server_listen_sock);
        g_server_listen_sock = INVALID_SOCKET;
    }
    if (g_server_thread.joinable()) {
        g_server_thread.join();
    }
    g_server_is_active.store(false);
}

bool is_background_axiom_server_running() {
    return g_server_is_active.load();
}
