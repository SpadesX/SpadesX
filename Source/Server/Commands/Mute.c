#include <Server/ParseConvert.h>
#include <Server/Server.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Notice.h>
#include <Util/Uthash.h>

void cmd_admin_mute(void* p_server, command_args_t arguments)
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
            if (player->admin_muted) {
                player->admin_muted = 0;
                send_server_notice(arguments.player, arguments.console, "%s has been admin unmuted", player->name);
            } else {
                player->admin_muted = 1;
                send_server_notice(arguments.player, arguments.console, "%s has been admin muted", player->name);
            }
        }
    } else {
        send_server_notice(arguments.player, arguments.console, "You did not enter ID or entered incorrect argument");
    }
}

void cmd_mute(void* p_server, command_args_t arguments)
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
            if (player->muted) {
                player->muted = 0;
                broadcast_server_notice(server, arguments.console, "%s has been unmuted", player->name);
            } else {
                player->muted = 1;
                broadcast_server_notice(server, arguments.console, "%s has been muted", player->name);
            }
        }
    } else {
        send_server_notice(arguments.player, arguments.console, "You did not enter ID or entered incorrect argument");
    }
}
