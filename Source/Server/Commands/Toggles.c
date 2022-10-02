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
        if (ID < server->protocol.max_players && is_past_join_screen(server, ID)) {
            if (server->player[ID].can_build == 1) {
                server->player[ID].can_build = 0;
                broadcast_server_notice(
                server, arguments.console, "Building has been disabled for %s", server->player[ID].name);
            } else if (server->player[ID].can_build == 0) {
                server->player[ID].can_build = 1;
                broadcast_server_notice(
                server, arguments.console, "Building has been enabled for %s", server->player[ID].name);
            }
        } else {
            send_server_notice(
            server, arguments.player_id, arguments.console, "ID not in range or player doesnt exist");
        }
    }
}

void cmd_toggle_kill(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    uint8_t   id;
    if (arguments.argc == 1) {
        if (server->global_ak == 1) {
            server->global_ak = 0;
            broadcast_server_notice(server, arguments.console, "Killing has been disabled");
        } else if (server->global_ak == 0) {
            server->global_ak = 1;
            broadcast_server_notice(server, arguments.console, "Killing has been enabled");
        }
    } else if (parse_player(arguments.argv[1], &id, NULL)) {
        if (id < server->protocol.max_players && is_past_join_screen(server, id)) {
            if (server->player[id].allow_killing == 1) {
                server->player[id].allow_killing = 0;
                broadcast_server_notice(
                server, arguments.console, "Killing has been disabled for %s", server->player[id].name);
            } else if (server->player[id].allow_killing == 0) {
                server->player[id].allow_killing = 1;
                broadcast_server_notice(
                server, arguments.console, "Killing has been enabled for %s", server->player[id].name);
            }
        } else {
            send_server_notice(
            server, arguments.player_id, arguments.console, "ID not in range or player doesnt exist");
        }
    }
}

void cmd_toggle_team_kill(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    uint8_t   ID     = 33;
    if (arguments.argc == 2 && parse_player(arguments.argv[1], &ID, NULL)) {
        if (ID < server->protocol.max_players && is_past_join_screen(server, ID)) {
            if (server->player[ID].allow_team_killing) {
                server->player[ID].allow_team_killing = 0;
                broadcast_server_notice(
                server, arguments.console, "Team killing has been disabled for %s", server->player[ID].name);
            } else {
                server->player[ID].allow_team_killing = 1;
                broadcast_server_notice(
                server, arguments.console, "Team killing has been enabled for %s", server->player[ID].name);
            }
        } else {
            send_server_notice(
            server, arguments.player_id, arguments.console, "ID not in range or player doesnt exist");
        }
    } else {
        send_server_notice(
        server, arguments.player_id, arguments.console, "You did not enter ID or entered incorrect argument");
    }
}
