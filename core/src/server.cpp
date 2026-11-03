#include "../include/ai.hpp"
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

#pragma comment(lib, "ws2_32.lib")

static std::string escape_json(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 16);
    for (char c : s) {
        switch (c) {
            case '\"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;      break;
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

// Extracts messages from Cline's JSON payload and formats into Qwen ChatML
static std::string build_qwen_chatml_prompt(const std::string& body) {
    std::string prompt;
    prompt.reserve(body.size() + 256);
    
    size_t cursor = 0;
    bool found_any_message = false;

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
            prompt += "<|im_start|>" + role + "\n" + content + "<|im_end|>\n";
            found_any_message = true;
        } else if (body[val_pos] == '[') {
            size_t text_key = body.find("\"text\"", val_pos);
            if (text_key != std::string::npos) {
                size_t text_colon = body.find(':', text_key + 6);
                if (text_colon != std::string::npos) {
                    size_t text_val_start = body.find('\"', text_colon + 1);
                    if (text_val_start != std::string::npos) {
                        std::string content = extract_json_string(body, text_val_start + 1, cursor);
                        prompt += "<|im_start|>" + role + "\n" + content + "<|im_end|>\n";
                        found_any_message = true;
                    }
                }
            }
            cursor = val_pos + 1;
        } else {
            cursor = val_pos + 1;
        }
    }

    if (!found_any_message) {
        prompt = "<|im_start|>system\nYou are Project Axiom, an expert C++23 and CUDA coding agent.<|im_end|>\n"
                 "<|im_start|>user\nWrite clean C++23 and CUDA code.<|im_end|>\n";
    }

    prompt += "<|im_start|>assistant\n";
    return prompt;
}

void run_axiom_openai_server(AI& engine, uint16_t port) {
    if (!engine.has_transformer_model()) {
        std::cout << "[..] Streaming Qwen 28-Layer weights via Win32 DMA mmap...\n";
        std::string weights_p = "models/qwen1_5b.axiom";
        std::string vocab_p = "models/qwen_vocab.axiom";
        if (std::filesystem::exists("core/models/qwen1_5b.axiom")) {
            weights_p = "core/models/qwen1_5b.axiom";
            vocab_p = "core/models/qwen_vocab.axiom";
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
                      << "[!] The server is already active and listening. To stop the existing server, run:\n"
                      << "    powershell \"Stop-Process -Name app -Force\"\n\n";
        } else {
            std::cerr << "[Axiom Server] bind failed on port " << port << " (WSA Error: " << err << ").\n";
        }
        closesocket(listen_sock);
        WSACleanup();
        return;
    }

    if (listen(listen_sock, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "[Axiom Server] listen failed.\n";
        closesocket(listen_sock);
        WSACleanup();
        return;
    }

    std::cout << "========================================================\n"
              << "     PROJECT AXIOM DUAL SERVER (OpenAI + Ollama API)    \n"
              << "========================================================\n"
              << "[OK] Listening on http://127.0.0.1:" << port << "\n"
              << "  - Cline VS Code  : http://127.0.0.1:" << port << "/v1\n"
              << "  - ComfyUI Ollama : http://127.0.0.1:" << port << "\n"
              << "--------------------------------------------------------\n"
              << "[Ready for requests... Press 'q' or Ctrl+C to stop]\n\n" << std::flush;

    fd_set read_fds;
    timeval tv{};

    while (true) {
        if (_isatty(_fileno(stdin)) && _kbhit()) {
            int ch = _getch();
            if (ch == 'q' || ch == 'Q' || ch == 27) {
                std::cout << "\n[Axiom Server] Shutdown requested ('" << (char)ch << "'). Closing server...\n";
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

        std::string req;
        char chunk[4096];
        int content_length = -1;
        size_t body_pos = std::string::npos;

        while (true) {
            int bytes = recv(client, chunk, sizeof(chunk), 0);
            if (bytes <= 0) break;
            req.append(chunk, bytes);

            if (body_pos == std::string::npos) {
                body_pos = req.find("\r\n\r\n");
                if (body_pos != std::string::npos) {
                    size_t cl_pos = req.find("Content-Length: ");
                    if (cl_pos == std::string::npos) cl_pos = req.find("content-length: ");
                    if (cl_pos != std::string::npos) {
                        content_length = std::stoi(req.substr(cl_pos + 16));
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
                std::cout << "[Server] GET /v1/models (Cline model discovery)\n" << std::flush;
                std::string models_json =
                    "{\"object\":\"list\",\"data\":["
                    "{\"id\":\"qwen2.5-coder-7b\",\"object\":\"model\",\"owned_by\":\"axiom\"},"
                    "{\"id\":\"qwen2.5-coder-1.5b\",\"object\":\"model\",\"owned_by\":\"axiom\"},"
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
                std::cout << "[Server] GET /api/tags (ComfyUI model discovery)\n" << std::flush;
                std::string tags_json =
                    "{\"models\":["
                    "{\"name\":\"huihui_ai/qwen2.5-vl-abliterated:7b\",\"model\":\"huihui_ai/qwen2.5-vl-abliterated:7b\",\"modified_at\":\"2026-11-03T00:00:00Z\",\"size\":3800000000,\"digest\":\"axiom\"},"
                    "{\"name\":\"axiom\",\"model\":\"axiom\",\"modified_at\":\"2026-11-03T00:00:00Z\",\"size\":3800000000,\"digest\":\"axiom\"},"
                    "{\"name\":\"qwen2.5-coder-7b\",\"model\":\"qwen2.5-coder-7b\",\"modified_at\":\"2026-11-03T00:00:00Z\",\"size\":3800000000,\"digest\":\"axiom\"},"
                    "{\"name\":\"qwen2.5-coder-1.5b\",\"model\":\"qwen2.5-coder-1.5b\",\"modified_at\":\"2026-11-03T00:00:00Z\",\"size\":3800000000,\"digest\":\"axiom\"}"
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
                std::cout << "[Server] POST /api/generate (ComfyUI prompt expansion)\n" << std::flush;
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
                    std::cout << tok << std::flush;
                });

                std::string ollama_resp =
                    "{\"model\":\"axiom\",\"response\":\"" + escape_json(generated_reply) + "\",\"done\":true,\"done_reason\":\"stop\"}";
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
                std::cout << "[Server] POST /v1/chat/completions (Cline token generation)\n" << std::flush;
                std::string chatml_prompt = build_qwen_chatml_prompt(body);
                bool is_stream = (body.find("\"stream\": true") != std::string::npos ||
                                  body.find("\"stream\":true") != std::string::npos);

                int max_tokens = 512;
                size_t mt_pos = body.find("\"max_tokens\":");
                if (mt_pos != std::string::npos) {
                    try {
                        int parsed = std::stoi(body.substr(mt_pos + 13));
                        if (parsed > 0 && parsed <= 2048) max_tokens = parsed;
                    } catch (...) {}
                }

                if (is_stream) {
                    std::string stream_hdr =
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/event-stream\r\n"
                        "Cache-Control: no-cache\r\n"
                        "Access-Control-Allow-Origin: *\r\n"
                        "Connection: close\r\n\r\n";
                    send(client, stream_hdr.data(), static_cast<int>(stream_hdr.size()), 0);

                    engine.generate(chatml_prompt, max_tokens, 0.2f, 40, 0.9f, [&](const std::string& tok) {
                        std::string chunk =
                            "data: {\"id\":\"chatcmpl-axiom\",\"object\":\"chat.completion.chunk\","
                            "\"choices\":[{\"index\":0,\"delta\":{\"content\":\"" + escape_json(tok) + "\"},\"finish_reason\":null}]}\n\n";
                        send(client, chunk.data(), static_cast<int>(chunk.size()), 0);
                        std::cout << tok << std::flush;
                    });

                    std::string end_chunk =
                        "data: {\"id\":\"chatcmpl-axiom\",\"object\":\"chat.completion.chunk\","
                        "\"choices\":[{\"index\":0,\"delta\":{},\"finish_reason\":\"stop\"}]}\n\n"
                        "data: [DONE]\n\n";
                    send(client, end_chunk.data(), static_cast<int>(end_chunk.size()), 0);
                } else {
                    std::string generated_reply;
                    engine.generate(chatml_prompt, max_tokens, 0.2f, 40, 0.9f, [&](const std::string& tok) {
                        generated_reply += tok;
                        std::cout << tok << std::flush;
                    });

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

    closesocket(listen_sock);
    WSACleanup();
}

