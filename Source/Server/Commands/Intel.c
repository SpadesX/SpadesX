#include <Server/Server.h>
#include <Util/Notice.h>

void cmd_intel(void* p_server, command_args_t arguments)
{
    server_t* server          = (server_t*) p_server;
    uint8_t   sentAtLeastOnce = 0;
    player_t *connected_player, *tmp;
    HASH_ITER(hh, server->players, connected_player, tmp)
    {
        if (connected_player->state != STATE_DISCONNECTED && connected_player->has_intel) {
            send_server_notice(arguments.player,
                               arguments.console,
                               "Player %s (#%hhu) has intel",
                               connected_player->name,
                               connected_player->id);
            sentAtLeastOnce = 1;
        }
    }
    if (sentAtLeastOnce == 0) {
        if (server->protocol.gamemode.intel_held[0]) {
            send_server_notice(
            arguments.player, arguments.console, "Intel is not being held but intel of team 0 thinks it is");
        } else if (server->protocol.gamemode.intel_held[1]) {
            send_server_notice(
            arguments.player, arguments.console, "Intel is not being held but intel of team 1 thinks it is");
        }
        send_server_notice(arguments.player, arguments.console, "Intel is not being held");
    }
}
