#include "../include/ai.hpp"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cstdint>

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

    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(port);

    if (bind(listen_sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "[Axiom Server] bind failed on port " << port << ".\n";
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

    std::cout << "[Axiom Server] Listening for Cline CLI on http://127.0.0.1:" << port << "/v1/chat/completions\n";

    while (true) {
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
            } else {
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

