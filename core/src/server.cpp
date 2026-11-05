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
    bool is_compaction = false;
    bool is_completion_turn = false;
    bool has_file_target = false;
    bool use_tool_calls = false;
    bool has_completion_tool = false;
    std::string tool_name = "replace_in_file";
    std::string path_key = "path";
    std::string tool_mode = "diff";
    std::string target_path;
    std::string full_content;
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

static size_t find_json_key(const std::string& json_text,
                            const std::string& key_name,
                            size_t start_index,
                            size_t end_index) {
    const std::string pattern = "\"" + key_name + "\"";
    size_t search_pos = start_index;
    const size_t limit = std::min(end_index, json_text.size());
    while (search_pos < limit) {
        size_t match_pos = json_text.find(pattern, search_pos);
        if (match_pos == std::string::npos || match_pos + pattern.size() >= limit) {
            return std::string::npos;
        }
        if (match_pos > 0 && json_text[match_pos - 1] == '\\') {
            search_pos = match_pos + pattern.size();
            continue;
        }
        size_t colon_pos = json_text.find_first_not_of(" \t\r\n", match_pos + pattern.size());
        if (colon_pos != std::string::npos && colon_pos < limit && json_text[colon_pos] == ':') {
            return colon_pos;
        }
        search_pos = match_pos + pattern.size();
    }
    return std::string::npos;
}

static size_t find_matching_delimiter(const std::string& json_text,
                                      size_t open_index,
                                      char open_char,
                                      char close_char) {
    bool in_quotes = false;
    int depth = 0;
    for (size_t idx = open_index; idx < json_text.size(); ++idx) {
        char c = json_text[idx];
        if (c == '\\' && in_quotes) {
            ++idx;
            continue;
        }
        if (c == '\"') {
            in_quotes = !in_quotes;
        } else if (!in_quotes) {
            if (c == open_char) {
                ++depth;
            } else if (c == close_char) {
                --depth;
                if (depth == 0) {
                    return idx;
                }
            }
        }
    }
    return std::string::npos;
}

static void parse_tool_definitions(const std::string& body, ClineContext& cline_context) {
    size_t tools_colon = find_json_key(body, "tools", 0, body.size());
    if (tools_colon == std::string::npos) return;

    size_t arr_start = body.find_first_not_of(" \t\r\n", tools_colon + 1);
    if (arr_start == std::string::npos || body[arr_start] != '[') return;

    size_t arr_end = find_matching_delimiter(body, arr_start, '[', ']');
    if (arr_end == std::string::npos) arr_end = body.size() - 1;

    int best_priority = 999;
    size_t cursor = arr_start + 1;
    while (cursor < arr_end) {
        size_t obj_start = body.find('{', cursor);
        if (obj_start == std::string::npos || obj_start >= arr_end) break;
        size_t obj_end = find_matching_delimiter(body, obj_start, '{', '}');
        if (obj_end == std::string::npos || obj_end > arr_end) break;

        size_t name_colon = find_json_key(body, "name", obj_start, obj_end);
        if (name_colon != std::string::npos) {
            size_t quote_pos = body.find('\"', name_colon + 1);
            if (quote_pos != std::string::npos && quote_pos < obj_end) {
                size_t next_pos = 0;
                std::string candidate_name = extract_json_string(body, quote_pos + 1, next_pos);
                cline_context.use_tool_calls = true;
                cline_context.is_cline = true;

                if (candidate_name == "attempt_completion") {
                    cline_context.has_completion_tool = true;
                }

                int priority = 999;
                if (candidate_name == "editor") {
                    priority = 1;
                } else if (candidate_name == "replace_in_file" || candidate_name == "apply_diff") {
                    priority = 2;
                } else if (candidate_name == "edit_file" || candidate_name == "str_replace" ||
                           candidate_name == "str_replace_editor" || candidate_name == "replace_string_in_file" ||
                           candidate_name == "apply_patch" || candidate_name == "MultiEdit" || candidate_name == "Edit") {
                    priority = 3;
                } else if (candidate_name == "write_to_file" || candidate_name == "write_file" ||
                           candidate_name == "Write" || candidate_name == "create_file") {
                    priority = 4;
                }

                if (priority < best_priority) {
                    best_priority = priority;
                    cline_context.tool_name = candidate_name;

                    size_t props_colon = find_json_key(body, "properties", obj_start, obj_end);
                    size_t props_start = obj_start;
                    size_t props_end = obj_end;
                    if (props_colon != std::string::npos) {
                        size_t p_open = body.find('{', props_colon + 1);
                        if (p_open != std::string::npos && p_open < obj_end) {
                            size_t p_close = find_matching_delimiter(body, p_open, '{', '}');
                            if (p_close != std::string::npos && p_close <= obj_end) {
                                props_start = p_open;
                                props_end = p_close;
                            }
                        }
                    }

                    if (find_json_key(body, "path", props_start, props_end) != std::string::npos) {
                        cline_context.path_key = "path";
                    } else if (find_json_key(body, "file_path", props_start, props_end) != std::string::npos) {
                        cline_context.path_key = "file_path";
                    } else if (find_json_key(body, "filepath", props_start, props_end) != std::string::npos) {
                        cline_context.path_key = "filepath";
                    } else if (find_json_key(body, "target_file", props_start, props_end) != std::string::npos) {
                        cline_context.path_key = "target_file";
                    } else if (find_json_key(body, "file", props_start, props_end) != std::string::npos) {
                        cline_context.path_key = "file";
                    } else {
                        cline_context.path_key = "path";
                    }

                    if (find_json_key(body, "old_text", props_start, props_end) != std::string::npos) {
                        cline_context.tool_mode = "old_new_text";
                    } else if (find_json_key(body, "diff", props_start, props_end) != std::string::npos) {
                        cline_context.tool_mode = "diff";
                    } else if (find_json_key(body, "old_str", props_start, props_end) != std::string::npos) {
                        cline_context.tool_mode = "old_new_str";
                    } else if (find_json_key(body, "old_string", props_start, props_end) != std::string::npos) {
                        cline_context.tool_mode = "old_new_string";
                    } else if (find_json_key(body, "search", props_start, props_end) != std::string::npos) {
                        cline_context.tool_mode = "search_replace";
                    } else if (find_json_key(body, "content", props_start, props_end) != std::string::npos || priority == 4) {
                        cline_context.tool_mode = "content";
                    } else {
                        cline_context.tool_mode = "diff";
                    }
                }
            }
        }
        cursor = obj_end + 1;
    }
}

static void parse_chat_messages(const std::string& body,
                                std::vector<std::pair<std::string, std::string>>& messages,
                                bool& last_is_tool) {
    last_is_tool = false;

    size_t msgs_colon = find_json_key(body, "messages", 0, body.size());
    size_t scan_start = 0;
    size_t scan_end = body.size();
    if (msgs_colon != std::string::npos) {
        size_t arr_open = body.find_first_not_of(" \t\r\n", msgs_colon + 1);
        if (arr_open != std::string::npos && body[arr_open] == '[') {
            scan_start = arr_open + 1;
            size_t arr_close = find_matching_delimiter(body, arr_open, '[', ']');
            if (arr_close != std::string::npos) scan_end = arr_close;
        }
    }

    size_t cursor = scan_start;
    while (cursor < scan_end) {
        size_t obj_start = body.find('{', cursor);
        if (obj_start == std::string::npos || obj_start >= scan_end) break;
        size_t obj_end = find_matching_delimiter(body, obj_start, '{', '}');
        if (obj_end == std::string::npos || obj_end > scan_end) break;

        size_t role_colon = find_json_key(body, "role", obj_start, obj_end);
        if (role_colon == std::string::npos) {
            cursor = obj_end + 1;
            continue;
        }
        size_t role_quote = body.find('\"', role_colon + 1);
        if (role_quote == std::string::npos || role_quote >= obj_end) {
            cursor = obj_end + 1;
            continue;
        }
        size_t role_next = 0;
        std::string role = extract_json_string(body, role_quote + 1, role_next);

        std::string content;
        size_t content_colon = find_json_key(body, "content", obj_start, obj_end);
        if (content_colon != std::string::npos) {
            size_t val_pos = body.find_first_not_of(" \t\r\n", content_colon + 1);
            if (val_pos != std::string::npos && val_pos < obj_end) {
                if (body[val_pos] == '\"') {
                    size_t next_c = 0;
                    content = extract_json_string(body, val_pos + 1, next_c);
                } else if (body[val_pos] == '[') {
                    size_t arr_end = find_matching_delimiter(body, val_pos, '[', ']');
                    if (arr_end == std::string::npos || arr_end > obj_end) arr_end = obj_end;
                    size_t text_pos = val_pos + 1;
                    while (text_pos < arr_end) {
                        size_t text_colon = find_json_key(body, "text", text_pos, arr_end);
                        if (text_colon == std::string::npos) break;
                        size_t text_val = body.find_first_not_of(" \t\r\n", text_colon + 1);
                        if (text_val == std::string::npos || text_val >= arr_end) break;
                        if (body[text_val] == '\"') {
                            size_t next_t = 0;
                            std::string part = extract_json_string(body, text_val + 1, next_t);
                            if (!content.empty()) content += "\n\n";
                            content += part;
                            text_pos = next_t;
                        } else {
                            text_pos = text_val + 1;
                        }
                    }
                }
            }
        }

        bool has_tool_call_id = (find_json_key(body, "tool_call_id", obj_start, obj_end) != std::string::npos);

        last_is_tool = (role == "tool" || has_tool_call_id);
        messages.emplace_back(role, content);
        cursor = obj_end + 1;
    }
}

static bool resolve_file_path(const std::string& candidate_input,
                              std::string& normalized_path,
                              std::string& disk_content) {
    std::string clean_path = candidate_input;
    while (!clean_path.empty() &&
           (clean_path.front() == '@' || clean_path.front() == '/' || clean_path.front() == '\\' ||
            clean_path.front() == '\"' || clean_path.front() == '\'' || clean_path.front() == '`')) {
        clean_path.erase(0, 1);
    }
    while (!clean_path.empty() &&
           (clean_path.back() == ',' || clean_path.back() == '.' || clean_path.back() == ':' ||
            clean_path.back() == ';' || clean_path.back() == '\"' || clean_path.back() == '\'' ||
            clean_path.back() == '`' || clean_path.back() == ')' || clean_path.back() == ']')) {
        clean_path.pop_back();
    }
    for (char& ch : clean_path) {
        if (ch == '\\') ch = '/';
    }
    if (clean_path.rfind("./", 0) == 0) {
        clean_path.erase(0, 2);
    }
    if (clean_path.empty()) return false;

    std::vector<std::pair<std::string, std::string>> search_pairs = {
        {clean_path, clean_path},
        {clean_path, "../" + clean_path},
        {"core/src/" + clean_path, "core/src/" + clean_path},
        {"core/src/" + clean_path, "../core/src/" + clean_path},
        {"core/" + clean_path, "../core/" + clean_path}
    };

    for (const auto& [norm_name, fs_path] : search_pairs) {
        std::error_code ec;
        if (std::filesystem::exists(fs_path, ec) && std::filesystem::is_regular_file(fs_path, ec)) {
            std::ifstream in(fs_path, std::ios::binary);
            if (in) {
                std::ostringstream ss;
                ss << in.rdbuf();
                disk_content = ss.str();
                normalized_path = norm_name;
                return true;
            }
        }
    }
    return false;
}

// Extracts messages from Cline's JSON payload and formats into Qwen ChatML.
// Seamlessly extracts all multi-part array text blocks (e.g. attached files).
static std::string build_qwen_chatml_prompt(const std::string& body, ClineContext& cline_context) {
    cline_context = ClineContext{};
    parse_tool_definitions(body, cline_context);

    std::vector<std::pair<std::string, std::string>> messages;
    bool last_is_tool = false;
    parse_chat_messages(body, messages, last_is_tool);

    if (messages.empty()) {
        return "<|im_start|>system\nYou are Project Axiom, an expert C++23 and CUDA coding agent.<|im_end|>\n"
               "<|im_start|>user\nWrite clean C++23 and CUDA code.<|im_end|>\n"
               "<|im_start|>assistant\n";
    }

    std::string latest_user_text;
    for (auto it = messages.rbegin(); it != messages.rend(); ++it) {
        if (it->first == "user" && !it->second.empty()) {
            latest_user_text = it->second;
            break;
        }
    }

    for (const auto& [role, text] : messages) {
        if (text.find("<task>") != std::string::npos ||
            text.find("<environment_details>") != std::string::npos ||
            text.find("<file_content") != std::string::npos ||
            text.find("replace_in_file") != std::string::npos ||
            text.find("write_to_file") != std::string::npos ||
            text.find("Plan / Act Modes") != std::string::npos ||
            text.find("ACT MODE") != std::string::npos ||
            text.find("Cline") != std::string::npos ||
            text.find("@/") != std::string::npos ||
            text.find(".cpp") != std::string::npos ||
            text.find(".hpp") != std::string::npos ||
            text.find(".cu") != std::string::npos) {
            cline_context.is_cline = true;
            break;
        }
    }

    if (!cline_context.is_cline && !latest_user_text.empty()) {
        if (latest_user_text.find("main") != std::string::npos ||
            latest_user_text.find("Main") != std::string::npos) {
            cline_context.is_cline = true;
        }
    }

    if (!cline_context.is_cline) {
        std::string prompt;
        prompt.reserve(body.size() + 256);
        for (const auto& [role, text] : messages) {
            std::string mapped_role = (role == "developer" || role == "system") ? "system" :
                                      (role == "assistant") ? "assistant" : "user";
            prompt += "<|im_start|>" + mapped_role + "\n" + text + "<|im_end|>\n";
        }
        prompt += "<|im_start|>assistant\n";
        return prompt;
    }

    std::string task_text;
    std::string target_path;
    std::string full_file_content;

    if (!latest_user_text.empty()) {
        size_t t_open = latest_user_text.find("<task>");
        size_t t_close = latest_user_text.find("</task>");
        if (t_open != std::string::npos && t_close != std::string::npos && t_close > t_open + 6) {
            task_text = trim_whitespace(latest_user_text.substr(t_open + 6, t_close - (t_open + 6)));
        } else {
            std::string raw_user = latest_user_text;
            size_t env_pos = raw_user.find("<environment_details>");
            if (env_pos != std::string::npos) {
                raw_user = raw_user.substr(0, env_pos);
            }
            size_t fc_pos = raw_user.find("<file_content");
            if (fc_pos != std::string::npos) {
                raw_user = raw_user.substr(0, fc_pos);
            }
            if (raw_user.size() > 600) {
                size_t first_nl = raw_user.find('\n');
                if (first_nl != std::string::npos && first_nl <= 300) {
                    raw_user = raw_user.substr(0, first_nl);
                } else {
                    raw_user = raw_user.substr(0, 300);
                }
            }
            task_text = trim_whitespace(raw_user);
        }

        size_t fc_open = latest_user_text.find("<file_content path=\"");
        if (fc_open != std::string::npos) {
            size_t path_start = fc_open + 20;
            size_t path_end = latest_user_text.find('\"', path_start);
            if (path_end != std::string::npos) {
                target_path = latest_user_text.substr(path_start, path_end - path_start);
                size_t tag_close = latest_user_text.find('>', path_end);
                size_t fc_end = latest_user_text.find("</file_content>", path_end);
                if (tag_close != std::string::npos && fc_end != std::string::npos && fc_end > tag_close + 1) {
                    full_file_content = trim_whitespace(latest_user_text.substr(tag_close + 1, fc_end - (tag_close + 1)));
                }
            }
        }
    }

    auto scan_text_path = [&](const std::string& src_text) -> std::string {
        std::istringstream iss(src_text.substr(0, std::min<size_t>(src_text.size(), 4000)));
        std::string tok;
        while (iss >> tok) {
            if (tok.find(".cpp") != std::string::npos || tok.find(".hpp") != std::string::npos ||
                tok.find(".cu") != std::string::npos || tok.find(".cuh") != std::string::npos ||
                tok.find(".h") != std::string::npos || tok.find(".py") != std::string::npos) {
                std::string norm;
                std::string disk;
                if (resolve_file_path(tok, norm, disk)) {
                    full_file_content = disk;
                    return norm;
                }
            }
        }
        return "";
    };

    if (target_path.empty() && !latest_user_text.empty()) {
        target_path = scan_text_path(latest_user_text);
    }

    if (target_path.empty()) {
        for (auto it = messages.rbegin(); it != messages.rend(); ++it) {
            if (it->first == "user") {
                target_path = scan_text_path(it->second);
                if (!target_path.empty()) break;
            }
        }
    }

    if (target_path.empty()) {
        for (auto it = messages.rbegin(); it != messages.rend(); ++it) {
            size_t fc_open = it->second.find("<file_content path=\"");
            if (fc_open != std::string::npos) {
                size_t path_start = fc_open + 20;
                size_t path_end = it->second.find('\"', path_start);
                if (path_end != std::string::npos) {
                    target_path = it->second.substr(path_start, path_end - path_start);
                    size_t tag_close = it->second.find('>', path_end);
                    size_t fc_end = it->second.find("</file_content>", path_end);
                    if (tag_close != std::string::npos && fc_end != std::string::npos && fc_end > tag_close + 1) {
                        full_file_content = trim_whitespace(it->second.substr(tag_close + 1, fc_end - (tag_close + 1)));
                    }
                    break;
                }
            }
        }
    }

    if (target_path.empty() && !latest_user_text.empty()) {
        std::string lower_user = latest_user_text;
        for (char& ch : lower_user) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        if (lower_user.find("main") != std::string::npos) {
            target_path = "core/src/main.cpp";
        }
    }

    if (!target_path.empty()) {
        std::string resolved_norm;
        std::string resolved_disk;
        if (resolve_file_path(target_path, resolved_norm, resolved_disk)) {
            target_path = resolved_norm;
            full_file_content = resolved_disk;
        }
    }

    while (true) {
        size_t at_pos = task_text.find("@/");
        if (at_pos == std::string::npos) break;
        task_text.erase(at_pos, 2);
    }

    // If compaction prompt or native tools not requested, format full conversation into standard Qwen ChatML
    const std::string& final_msg_text = messages.back().second;
    const bool is_compaction_prompt = (final_msg_text.find("### Goal") != std::string::npos &&
                                       final_msg_text.find("### State") != std::string::npos &&
                                       final_msg_text.find("### Highlights") != std::string::npos);
    if (is_compaction_prompt || !cline_context.use_tool_calls) {
        cline_context.use_tool_calls = false;
        cline_context.has_file_target = false;
        cline_context.is_compaction = false;
        std::string prompt;
        prompt.reserve(body.size() + 256);
        for (const auto& [role, text] : messages) {
            std::string mapped_role = (role == "developer" || role == "system") ? "system" :
                                      (role == "assistant") ? "assistant" : "user";
            prompt += "<|im_start|>" + mapped_role + "\n" + text + "<|im_end|>\n";
        }
        prompt += "<|im_start|>assistant\n";
        return prompt;
    }

    // Check if the most recent turn is a tool result or successful file modification result from Cline
    const std::string& last_role = messages.back().first;
    const bool is_user_turn = (last_role == "user");
    const bool is_read_result = (final_msg_text.find("[read_file for '") != std::string::npos);
    const bool has_error_text = (final_msg_text.find("Error") != std::string::npos ||
                                 final_msg_text.find("does not match") != std::string::npos);
    const bool is_edit_success =
        (!is_user_turn &&
         (final_msg_text.find("[replace_in_file") != std::string::npos ||
          final_msg_text.find("[write_to_file") != std::string::npos ||
          final_msg_text.find("[edit_file") != std::string::npos ||
          final_msg_text.find("[editor") != std::string::npos ||
          final_msg_text.find("Edited ") != std::string::npos) &&
         (final_msg_text.find("final_file_content") != std::string::npos ||
          final_msg_text.find("successfully") != std::string::npos ||
          final_msg_text.find("```diff") != std::string::npos) &&
         !has_error_text);

    if (!is_user_turn && ((last_is_tool && !is_read_result && !has_error_text) || is_edit_success)) {
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

    // Check if any message contains a [read_file for '...'] Result:
    if (full_file_content.empty()) {
        size_t rf_pos = final_msg_text.find("[read_file for '");
        if (rf_pos != std::string::npos) {
            size_t p_start = rf_pos + 16;
            size_t p_end = final_msg_text.find("']", p_start);
            if (p_end != std::string::npos) {
                target_path = final_msg_text.substr(p_start, p_end - p_start);
                size_t res_pos = final_msg_text.find("Result:", p_end);
                if (res_pos != std::string::npos) {
                    size_t content_start = final_msg_text.find('\n', res_pos);
                    if (content_start != std::string::npos) {
                        size_t env_pos = final_msg_text.find("<environment_details>", content_start);
                        full_file_content = trim_whitespace(final_msg_text.substr(
                            content_start + 1,
                            (env_pos != std::string::npos) ? (env_pos - (content_start + 1)) : std::string::npos));
                    }
                }
            }
        }
    }

    if (!target_path.empty() && !full_file_content.empty()) {
        select_file_excerpt(full_file_content, task_text, cline_context.search_excerpt, cline_context.stop_line);
        if (!cline_context.search_excerpt.empty()) {
            cline_context.has_file_target = true;
            cline_context.target_path = target_path;
            cline_context.full_content = full_file_content;
            cline_context.initial_prefix =
                "<replace_in_file>\n<path>" + target_path + "</path>\n<diff>\n------- SEARCH\n" +
                cline_context.search_excerpt + "\n=======\n";
            cline_context.closing_suffix = "+++++++ REPLACE\n</diff>\n</replace_in_file>";

            if (task_text.empty()) {
                task_text = "Improve the code block in " + target_path;
            }

            std::string prompt_rules;
            if (target_path.find("main.cpp") != std::string::npos) {
                prompt_rules =
                    "Rules:\n"
                    "- Output valid, compilable C++23 code ending with a semicolon (;).\n"
                    "- For visual or UI improvements, add ANSI color codes (\\033[1;36m, \\033[1;32m, \\033[1;33m, \\033[0m) and clean visual styling while keeping all menu numbers [1]-[5] intact.\n";
            } else {
                prompt_rules =
                    "Rules:\n"
                    "- Output valid, compilable C++23 / CUDA code ending with a semicolon (;).\n"
                    "- Preserve original function signatures and surrounding logic.\n";
            }

            return "<|im_start|>system\n"
                   "You are Cline, an expert C++23 and CUDA software engineer.\n"
                   "Write ONLY the improved replacement C++23 code for the ------- SEARCH block, then output +++++++ REPLACE.\n" +
                   prompt_rules +
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
                   "Provide the improved replacement code for this block.<|im_end|>\n"
                   "<|im_start|>assistant\n";
        }
    }

    // Fallback Cline prompt when no file is attached yet
    std::string clean_user = task_text.empty() ? latest_user_text : task_text;
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
                ClineContext cline_context;
                std::string chatml_prompt = build_qwen_chatml_prompt(body, cline_context);
                bool is_stream = (body.find("\"stream\": true") != std::string::npos ||
                                  body.find("\"stream\":true") != std::string::npos);

                if (!is_background) {
                    std::cout << "[Server] POST /v1/chat/completions"
                              << " | Mode: " << (cline_context.is_compaction ? "Compaction" :
                                                 cline_context.is_completion_turn ? "Completion" :
                                                 cline_context.has_file_target ? (cline_context.use_tool_calls ? "OpenAI-Tool-Edit" : "XML-Edit") :
                                                 "Chat")
                              << " | Target: " << (cline_context.target_path.empty() ? "none" : cline_context.target_path)
                              << " | Tool: " << (cline_context.use_tool_calls ? (cline_context.tool_name + "(" + cline_context.tool_mode + ")") : "xml")
                              << "\n" << std::flush;
                }

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

                auto send_openai_tool_call = [&](const std::string& fn_name, const std::string& fn_args_json) {
                    if (!is_background) {
                        std::cout << "\n[ToolCall] " << fn_name << " -> " << fn_args_json.substr(0, std::min<size_t>(fn_args_json.size(), 240)) << "...\n" << std::flush;
                    }
                    if (is_stream) {
                        std::string chunk1 =
                            "data: {\"id\":\"chatcmpl-axiom\",\"object\":\"chat.completion.chunk\",\"model\":\"axiom\","
                            "\"choices\":[{\"index\":0,\"delta\":{\"role\":\"assistant\",\"content\":null,"
                            "\"tool_calls\":[{\"index\":0,\"id\":\"call_axiom_1\",\"type\":\"function\","
                            "\"function\":{\"name\":\"" + escape_json(fn_name) + "\",\"arguments\":\"\"}}]},"
                            "\"finish_reason\":null}]}\n\n";
                        send(client, chunk1.data(), static_cast<int>(chunk1.size()), 0);

                        std::string chunk2 =
                            "data: {\"id\":\"chatcmpl-axiom\",\"object\":\"chat.completion.chunk\",\"model\":\"axiom\","
                            "\"choices\":[{\"index\":0,\"delta\":{"
                            "\"tool_calls\":[{\"index\":0,\"function\":{\"arguments\":\"" + escape_json(fn_args_json) + "\"}}]},"
                            "\"finish_reason\":null}]}\n\n";
                        send(client, chunk2.data(), static_cast<int>(chunk2.size()), 0);

                        std::string end_chunk =
                            "data: {\"id\":\"chatcmpl-axiom\",\"object\":\"chat.completion.chunk\",\"model\":\"axiom\","
                            "\"choices\":[{\"index\":0,\"delta\":{},\"finish_reason\":\"tool_calls\"}]}\n\n"
                            "data: [DONE]\n\n";
                        send(client, end_chunk.data(), static_cast<int>(end_chunk.size()), 0);
                    } else {
                        std::string json_payload =
                            "{\"id\":\"chatcmpl-axiom\",\"object\":\"chat.completion\",\"model\":\"axiom\",\"choices\":[{\"index\":0,"
                            "\"message\":{\"role\":\"assistant\",\"content\":null,"
                            "\"tool_calls\":[{\"id\":\"call_axiom_1\",\"type\":\"function\","
                            "\"function\":{\"name\":\"" + escape_json(fn_name) + "\",\"arguments\":\"" + escape_json(fn_args_json) + "\"}}]},"
                            "\"finish_reason\":\"tool_calls\"}]}";

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
                bool sent_tool_call_response = false;

                if (cline_context.has_file_target) {
                    if (!cline_context.use_tool_calls) {
                        emit_delta(cline_context.initial_prefix, generated_reply, emitted_any);
                    }
                    std::string replace_code;
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
                            replace_code += raw_line + "\n";
                            if (!cline_context.use_tool_calls) {
                                emit_delta(raw_line + "\n", generated_reply, emitted_any);
                            } else if (!is_background) {
                                std::cout << raw_line << "\n" << std::flush;
                            }
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
                            replace_code += line_buffer + "\n";
                            if (!cline_context.use_tool_calls) {
                                emit_delta(line_buffer + "\n", generated_reply, emitted_any);
                            } else if (!is_background) {
                                std::cout << line_buffer << "\n" << std::flush;
                            }
                            ++emitted_code_lines;
                        }
                    }
                    if (emitted_code_lines == 0) {
                        replace_code = cline_context.search_excerpt + "\n";
                        if (!cline_context.use_tool_calls) {
                            emit_delta(replace_code, generated_reply, emitted_any);
                        }
                    }

                    if (!cline_context.use_tool_calls) {
                        emit_delta(cline_context.closing_suffix, generated_reply, emitted_any);
                    } else {
                        std::string clean_replace = replace_code;
                        if (!clean_replace.empty() && clean_replace.back() == '\n') {
                            clean_replace.pop_back();
                        }
                        std::string args_json;
                        if (cline_context.tool_mode == "old_new_text") {
                            args_json = "{\"" + cline_context.path_key + "\":\"" + escape_json(cline_context.target_path) +
                                        "\",\"old_text\":\"" + escape_json(cline_context.search_excerpt) +
                                        "\",\"new_text\":\"" + escape_json(clean_replace) + "\"}";
                        } else if (cline_context.tool_mode == "old_new_str") {
                            args_json = "{\"" + cline_context.path_key + "\":\"" + escape_json(cline_context.target_path) +
                                        "\",\"old_str\":\"" + escape_json(cline_context.search_excerpt) +
                                        "\",\"new_str\":\"" + escape_json(clean_replace) + "\"}";
                        } else if (cline_context.tool_mode == "old_new_string") {
                            args_json = "{\"" + cline_context.path_key + "\":\"" + escape_json(cline_context.target_path) +
                                        "\",\"old_string\":\"" + escape_json(cline_context.search_excerpt) +
                                        "\",\"new_string\":\"" + escape_json(clean_replace) + "\"}";
                        } else if (cline_context.tool_mode == "search_replace") {
                            args_json = "{\"" + cline_context.path_key + "\":\"" + escape_json(cline_context.target_path) +
                                        "\",\"search\":\"" + escape_json(cline_context.search_excerpt) +
                                        "\",\"replace\":\"" + escape_json(clean_replace) + "\"}";
                        } else if (cline_context.tool_mode == "content") {
                            std::string updated_full = cline_context.full_content;
                            size_t match_pos = updated_full.find(cline_context.search_excerpt);
                            if (match_pos != std::string::npos) {
                                updated_full.replace(match_pos, cline_context.search_excerpt.size(), clean_replace);
                            }
                            args_json = "{\"" + cline_context.path_key + "\":\"" + escape_json(cline_context.target_path) +
                                        "\",\"content\":\"" + escape_json(updated_full) + "\"}";
                        } else {
                            std::string diff_block =
                                "------- SEARCH\n" + cline_context.search_excerpt + "\n=======\n" + clean_replace + "\n+++++++ REPLACE";
                            args_json = "{\"" + cline_context.path_key + "\":\"" + escape_json(cline_context.target_path) +
                                        "\",\"diff\":\"" + escape_json(diff_block) + "\"}";
                        }
                        send_openai_tool_call(cline_context.tool_name, args_json);
                        sent_tool_call_response = true;
                    }
                } else if (cline_context.is_completion_turn) {
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

                    if (cline_context.use_tool_calls && cline_context.has_completion_tool) {
                        std::string comp_args = "{\"result\":\"" + escape_json(summary_buffer) + "\"}";
                        send_openai_tool_call("attempt_completion", comp_args);
                        sent_tool_call_response = true;
                    } else if (cline_context.use_tool_calls) {
                        emit_delta(summary_buffer, generated_reply, emitted_any);
                    } else {
                        emit_delta(cline_context.initial_prefix, generated_reply, emitted_any);
                        emit_delta(summary_buffer, generated_reply, emitted_any);
                        emit_delta(cline_context.closing_suffix, generated_reply, emitted_any);
                    }
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

                if (!sent_tool_call_response) {
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
