#ifndef SERVER_HPP
#define SERVER_HPP

#include <cstdint>
#include "ai.hpp"

void run_axiom_openai_server(AI& engine, uint16_t port);
bool start_background_axiom_server(AI& engine, uint16_t port);
void stop_background_axiom_server();
bool is_background_axiom_server_running();

#endif // SERVER_HPP

