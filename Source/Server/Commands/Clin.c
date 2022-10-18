#include <Server/ParseConvert.h>
#include <Server/Server.h>
#include <Server/Structs/ServerStruct.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Notice.h>
#include <stdio.h>

void cmd_clin(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    uint8_t player_id = 33;
    if (arguments.argc > 2) {
        send_server_notice(arguments.player, arguments.console, "Too many arguments");
        return;
    } else if (arguments.argc != 2 && arguments.console) {
        send_server_notice(arguments.player, arguments.console, "No ID given");
        return;
    } else if (arguments.argc == 2 && !parse_player(arguments.argv[1], &player_id, NULL)) {
        send_server_notice(arguments.player, arguments.console, "Invalid ID. Wrong format");
        return;
    }

    if (arguments.player->id < server->protocol.max_players && is_past_join_screen(arguments.player)) {
        char client[15];
        if (arguments.player->client == 'o') {
            snprintf(client, 12, "OpenSpades");
        } else if (arguments.player->client == 'B') {
            snprintf(client, 13, "BetterSpades");
        } else {
            snprintf(client, 7, "Voxlap");
        }
        send_server_notice(arguments.player,
                           arguments.console,
                           "Player %s is running %s version %d.%d.%d on %s",
                           arguments.player->name,
                           client,
                           arguments.player->version_major,
                           arguments.player->version_minor,
                           arguments.player->version_revision,
                           arguments.player->os_info);
    } else {
        send_server_notice(arguments.player, arguments.console, "Invalid ID. Player doesnt exist");
    }
}
