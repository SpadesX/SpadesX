#include <Server/ParseConvert.h>
#include <Server/Server.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Notice.h>

void cmd_toggle_build(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    uint8_t   ID;
    if (arguments.argc == 1) {
        if (server->global_ab == 1) {
            server->global_ab = 0;
            broadcast_server_notice(server, arguments.console, "Building has been disabled");
        } else if (server->global_ab == 0) {
            server->global_ab = 1;
            broadcast_server_notice(server, arguments.console, "Building has been enabled");
        }
    } else if (parse_player(arguments.argv[1], &ID, NULL)) {
        player_t* player;
        HASH_FIND(hh, server->players, &ID, sizeof(ID), player);
        if (player == NULL) {
            send_server_notice(arguments.player, arguments.console, "ID not in range or player doesnt exist");
            return;
        }
        if (is_past_join_screen(player)) {
            if (player->can_build == 1) {
                player->can_build = 0;
                broadcast_server_notice(server, arguments.console, "Building has been disabled for %s", player->name);
            } else if (player->can_build == 0) {
                player->can_build = 1;
                broadcast_server_notice(server, arguments.console, "Building has been enabled for %s", player->name);
            }
        }
    }
}

void cmd_toggle_kill(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    uint8_t   ID;
    if (arguments.argc == 1) {
        if (server->global_ak == 1) {
            server->global_ak = 0;
            broadcast_server_notice(server, arguments.console, "Killing has been disabled");
        } else if (server->global_ak == 0) {
            server->global_ak = 1;
            broadcast_server_notice(server, arguments.console, "Killing has been enabled");
        }
    } else if (parse_player(arguments.argv[1], &ID, NULL)) {
        player_t* player;
        HASH_FIND(hh, server->players, &ID, sizeof(ID), player);
        if (player == NULL) {
            send_server_notice(arguments.player, arguments.console, "ID not in range or player doesnt exist");
            return;
        }
        if (is_past_join_screen(player)) {
            if (player->allow_killing == 1) {
                player->allow_killing = 0;
                broadcast_server_notice(server, arguments.console, "Killing has been disabled for %s", player->name);
            } else if (player->allow_killing == 0) {
                player->allow_killing = 1;
                broadcast_server_notice(server, arguments.console, "Killing has been enabled for %s", player->name);
            }
        }
    }
}

void cmd_toggle_team_kill(void* p_server, command_args_t arguments)
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
            if (player->allow_team_killing) {
                player->allow_team_killing = 0;
                broadcast_server_notice(
                server, arguments.console, "Team killing has been disabled for %s", player->name);
            } else {
                player->allow_team_killing = 1;
                broadcast_server_notice(
                server, arguments.console, "Team killing has been enabled for %s", player->name);
            }
        } else {
            send_server_notice(arguments.player, arguments.console, "ID not in range or player doesnt exist");
        }
    }
}
