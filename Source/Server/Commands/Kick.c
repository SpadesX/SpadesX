#include <Server/ParseConvert.h>
#include <Server/Server.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Notice.h>
#include <Util/Uthash.h>

void cmd_kick(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    uint8_t   ID     = 33;
    if (arguments.argc == 2 && parse_player(arguments.argv[1], &ID, NULL)) {
        player_t* player;
        HASH_FIND(hh, server->players, &ID, sizeof(ID), player);
        if (player == NULL) {
            send_server_notice(arguments.player, arguments.console, "ID not in range or player doesnt exist");
            return;
        }
        if (is_past_join_screen(player)) {
            enet_peer_disconnect(player->peer, REASON_KICKED);
            broadcast_server_notice(
            server, arguments.console, "Player %s (#%hhu) has been kicked", player->name, player->id);
        }
    } else {
        send_server_notice(arguments.player, arguments.console, "You did not enter ID or entered incorrect argument");
    }
}
