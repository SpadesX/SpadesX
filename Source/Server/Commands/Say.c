#include <Server/Server.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Notice.h>

void cmd_say(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    if (arguments.argc == 2) {
        player_t *connected_player, *tmp;
        HASH_ITER(hh, server->players, connected_player, tmp)
        {
            if (is_past_join_screen(connected_player)) {
                send_server_notice(connected_player, 0, arguments.argv[1]);
            }
        }
    } else {
        send_server_notice(arguments.player, arguments.console, "Invalid message");
    }
}
