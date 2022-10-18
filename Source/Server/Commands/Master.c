#include <Server/Master.h>
#include <Server/Server.h>
#include <Util/Notice.h>

void cmd_master(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    if (server->master.enable_master_connection == 1) {
        server->master.enable_master_connection = 0;
        enet_host_destroy(server->master.client);
        send_server_notice(arguments.player, arguments.console, "Disabling master connection");
        return;
    }
    server->master.enable_master_connection = 1;
    master_connect(server, server->port);
    send_server_notice(arguments.player, arguments.console, "Enabling master connection");
}
