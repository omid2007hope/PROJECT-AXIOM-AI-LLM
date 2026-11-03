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

// Extracts the last user message from Cline's JSON payload and wraps in Qwen ChatML
static std::string build_qwen_chatml_prompt(const std::string& body) {
    std::string content = "Write clean C++23 and CUDA code.";
    size_t pos = body.rfind("\"content\"");
    if (pos != std::string::npos) {
        size_t start = body.find('\"', pos + 9);
        if (start != std::string::npos) {
            size_t end = body.find("\"}", start + 1);
            if (end == std::string::npos) {
                end = body.find("\"", start + 1);
            }
            if (end != std::string::npos) {
                content = body.substr(start + 1, end - start - 1);
            }
        }
    }
    return "<|im_start|>system\nYou are Project Axiom, an expert C++23 and CUDA coding agent.<|im_end|>\n"
           "<|im_start|>user\n" + content + "<|im_end|>\n"
           "<|im_start|>assistant\n";
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

            // Handle GET /models
            if (req.rfind("GET /v1/models", 0) == 0 || req.rfind("GET /models", 0) == 0) {
                std::string models_json = "{\"object\":\"list\",\"data\":[{\"id\":\"qwen2.5-coder-1.5b\",\"object\":\"model\",\"owned_by\":\"axiom\"}]}";
                std::ostringstream resp;
                resp << "HTTP/1.1 200 OK\r\n"
                     << "Content-Type: application/json\r\n"
                     << "Content-Length: " << models_json.size() << "\r\n"
                     << "Connection: close\r\n\r\n"
                     << models_json;
                std::string wire = resp.str();
                send(client, wire.data(), static_cast<int>(wire.size()), 0);
            } else {
                std::string chatml_prompt = build_qwen_chatml_prompt(body);
                std::string generated_reply;

                engine.generate(chatml_prompt, 512, 0.2f, 40, 0.9f, [&](const std::string& tok) {
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
                     << "Content-Length: " << json_payload.size() << "\r\n"
                     << "Connection: close\r\n\r\n"
                     << json_payload;

                std::string wire = resp.str();
                send(client, wire.data(), static_cast<int>(wire.size()), 0);
            }
        }

        closesocket(client);
    }

    closesocket(listen_sock);
    WSACleanup();
}

