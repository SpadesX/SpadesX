#include <Server/Commands/Commands.h>
#include <Server/Server.h>
#include <Util/Notice.h>
#include <Util/Utlist.h>

void cmd_help(void* p_server, command_args_t arguments)
{
    server_t*  server = (server_t*) p_server;
    command_t* cmd    = NULL;
    if (arguments.console)
        send_server_notice(server, arguments.player_id, arguments.console, "Commands available to you:");

    LL_FOREACH(server->cmds_list, cmd)
    {
        if (cmd == NULL) {
            return; // Just for safety
        }
        if (player_has_permission(server, arguments.player_id, arguments.console, cmd->permissions) ||
            cmd->permissions == 0)
        {
            send_server_notice(server,
                               arguments.player_id,
                               arguments.console,
                               "[Command: %s, Description: %s]",
                               cmd->id,
                               cmd->description);
        }
    }
    if (!arguments.console)
        send_server_notice(server, arguments.player_id, arguments.console, "Commands available to you:");
}
