#include <Server/Server.h>

void cmd_shutdown(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    (void) arguments;

    server->running = 0;
}
