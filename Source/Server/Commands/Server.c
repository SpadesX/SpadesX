#include <Server/Server.h>
#include <Util/Notice.h>

void cmd_server(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    send_server_notice(
    server, arguments.player_id, arguments.console, "You are playing on SpadesX server. Version %s", VERSION);
}
