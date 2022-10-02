#include <Server/ParseConvert.h>
#include <Server/Server.h>
#include <Server/Structs/ServerStruct.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Notice.h>
#include <stdio.h>

void cmd_clin(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    uint8_t   ID     = arguments.player_id;

    if (arguments.argc > 2) {
        send_server_notice(server, arguments.player_id, arguments.console, "Too many arguments");
        return;
    } else if (arguments.argc != 2 && arguments.console) {
        send_server_notice(server, arguments.player_id, arguments.console, "No ID given");
        return;
    } else if (arguments.argc == 2 && !parse_player(arguments.argv[1], &ID, NULL)) {
        send_server_notice(server, arguments.player_id, arguments.console, "Invalid ID. Wrong format");
        return;
    }

    if (ID < server->protocol.max_players && is_past_join_screen(server, ID)) {
        char client[15];
        if (server->player[ID].client == 'o') {
            snprintf(client, 12, "OpenSpades");
        } else if (server->player[ID].client == 'B') {
            snprintf(client, 13, "BetterSpades");
        } else {
            snprintf(client, 7, "Voxlap");
        }
        send_server_notice(server,
                           arguments.player_id,
                           arguments.console,
                           "Player %s is running %s version %d.%d.%d on %s",
                           server->player[ID].name,
                           client,
                           server->player[ID].version_major,
                           server->player[ID].version_minor,
                           server->player[ID].version_revision,
                           server->player[ID].os_info);
    } else {
        send_server_notice(server, arguments.player_id, arguments.console, "Invalid ID. Player doesnt exist");
    }
}
