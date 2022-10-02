#include <Server/ParseConvert.h>
#include <Server/Server.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Notice.h>

void cmd_admin_mute(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    uint8_t   ID     = 33;
    if (arguments.argc == 2 && parse_player(arguments.argv[1], &ID, NULL)) {
        if (ID < server->protocol.max_players && is_past_join_screen(server, ID)) {
            if (server->player[ID].admin_muted) {
                server->player[ID].admin_muted = 0;
                send_server_notice(
                server, arguments.player_id, arguments.console, "%s has been admin unmuted", server->player[ID].name);
            } else {
                server->player[ID].admin_muted = 1;
                send_server_notice(
                server, arguments.player_id, arguments.console, "%s has been admin muted", server->player[ID].name);
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

void cmd_mute(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    uint8_t   ID     = 33;
    if (arguments.argc == 2 && parse_player(arguments.argv[1], &ID, NULL)) {
        if (ID < server->protocol.max_players && is_past_join_screen(server, ID)) {
            if (server->player[ID].muted) {
                server->player[ID].muted = 0;
                broadcast_server_notice(server, arguments.console, "%s has been unmuted", server->player[ID].name);
            } else {
                server->player[ID].muted = 1;
                broadcast_server_notice(server, arguments.console, "%s has been muted", server->player[ID].name);
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
