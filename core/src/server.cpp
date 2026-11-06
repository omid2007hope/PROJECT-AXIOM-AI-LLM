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
#include <algorithm>

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif

static std::atomic<bool> g_server_stop_requested{false};
static std::atomic<bool> g_server_is_active{false};
static std::thread g_server_thread;
static SOCKET g_server_listen_sock = INVALID_SOCKET;

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

static void parse_chat_messages(const std::string& body,
                                std::vector<std::pair<std::string, std::string>>& messages) {
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

        messages.emplace_back(role, content);
        cursor = obj_end + 1;
    }
}

static std::string build_qwen_chatml_prompt(const std::string& body) {
    std::vector<std::pair<std::string, std::string>> messages;
    parse_chat_messages(body, messages);

    if (messages.empty()) {
        return "<|im_start|>system\nYou are Project Axiom, a bare-metal C++23 and CUDA AI engine.<|im_end|>\n"
               "<|im_start|>user\nHello<|im_end|>\n"
               "<|im_start|>assistant\n";
    }

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
                  << "        PROJECT AXIOM HTTP SERVER (OpenAI API)          \n"
                  << "========================================================\n"
                  << "[OK] Listening on http://127.0.0.1:" << port << "\n"
                  << "  - Models    : GET  http://127.0.0.1:" << port << "/v1/models\n"
                  << "  - Chat/Comp : POST http://127.0.0.1:" << port << "/v1/chat/completions\n"
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
                if (!is_background) std::cout << "[Server] GET /v1/models\n" << std::flush;
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
            // Handle POST /chat/completions
            else if (req.rfind("POST /v1/chat/completions", 0) == 0 || req.rfind("POST /chat/completions", 0) == 0) {
                if (!is_background) std::cout << "[Server] POST /v1/chat/completions\n" << std::flush;
                std::string chatml_prompt = build_qwen_chatml_prompt(body);
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

                engine.generate(chatml_prompt, max_tokens, 0.2f, 40, 0.9f, [&](const std::string& tok) {
                    if (tok.find("<|im_start|>") != std::string::npos ||
                        tok.find("<|im_end|>") != std::string::npos ||
                        tok.find("<|endoftext|>") != std::string::npos) {
                        return;
                    }
                    emit_delta(tok, generated_reply, emitted_any);
                });

                if (!emitted_any) {
                    emit_delta("I have reviewed your request.", generated_reply, emitted_any);
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
            } else {
                std::string not_found =
                    "HTTP/1.1 404 Not Found\r\n"
                    "Content-Type: application/json\r\n"
                    "Connection: close\r\n\r\n"
                    "{\"error\":\"Not Found\"}";
                send(client, not_found.data(), static_cast<int>(not_found.size()), 0);
            }
        }

        closesocket(client);
    }

    closesocket(listen_sock);
    g_server_listen_sock = INVALID_SOCKET;
    WSACleanup();
    g_server_is_active.store(false);
    if (!is_background) {
        std::cout << "[OK] Axiom HTTP server stopped cleanly.\n";
    }
}

void run_axiom_openai_server(AI& engine, uint16_t port) {
    g_server_stop_requested.store(false);
    g_server_is_active.store(true);
    execute_server_loop(engine, port, false);
}

bool start_background_axiom_server(AI& engine, uint16_t port) {
    if (g_server_is_active.load()) {
        std::cout << "[!] Server is already running in the background.\n";
        return false;
    }

    g_server_stop_requested.store(false);
    g_server_is_active.store(true);

    g_server_thread = std::thread([&engine, port]() {
        execute_server_loop(engine, port, true);
    });
    g_server_thread.detach();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    return true;
}

void stop_background_axiom_server() {
    if (!g_server_is_active.load()) {
        return;
    }
    g_server_stop_requested.store(true);

    if (g_server_listen_sock != INVALID_SOCKET) {
        closesocket(g_server_listen_sock);
        g_server_listen_sock = INVALID_SOCKET;
    }

    int wait_cycles = 0;
    while (g_server_is_active.load() && wait_cycles < 20) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        wait_cycles++;
    }
}

bool is_background_axiom_server_running() {
    return g_server_is_active.load();
}
