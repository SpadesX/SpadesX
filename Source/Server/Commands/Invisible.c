#include <Server/Packets/Packets.h>
#include <Server/Server.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Log.h>
#include <Util/Notice.h>

void cmd_inv(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    if (arguments.console) {
        LOG_INFO("You cannot use this command from console");
        return;
    }
    if (arguments.player->is_invisible == 1) {
        arguments.player->is_invisible  = 0;
        arguments.player->allow_killing = 1;
        player_t *connected_player, *tmp;
        HASH_ITER(hh, server->players, connected_player, tmp)
        {
            if (is_past_join_screen(connected_player) && connected_player != arguments.player) {
                send_create_player(server, connected_player, arguments.player);
            }
        }
        send_server_notice(arguments.player, arguments.console, "You are no longer invisible");
    } else if (arguments.player->is_invisible == 0) {
        arguments.player->is_invisible  = 1;
        arguments.player->allow_killing = 0;
        send_kill_action_packet(server, arguments.player, arguments.player, 0, 0, 1);
        send_server_notice(arguments.player, arguments.console, "You are now invisible");
    }
}
