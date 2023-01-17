#include <Server/Server.h>
#include <Util/Notice.h>

void cmd_server(void* p_server, command_args_t arguments)
{
    (void) p_server;
    send_server_notice(arguments.player, arguments.console, "You are playing on SpadesX server. Version %s", VERSION);
}
