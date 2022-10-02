#include <Server/Server.h>

void cmd_reset(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    (void) arguments;
    for (uint32_t i = 0; i < server->protocol.max_players; ++i) {
        if (server->player[i].state != STATE_DISCONNECTED) {
            server->player[i].state = STATE_STARTING_MAP;
        }
    }
    server_reset(server);
}
