#include <Server/ParseConvert.h>
#include <Server/Server.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Notice.h>

void cmd_pm(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    char*     PM;
    uint8_t   ID = 33;
    if (arguments.argc == 2 && parse_player(arguments.argv[1], &ID, &PM) && strlen(++PM) > 0) {
        player_t* player;
        HASH_FIND(hh, server->players, &ID, sizeof(ID), player);
        if (player == NULL) {
            send_server_notice(arguments.player, arguments.console, "ID not in range or player doesnt exist");
            return;
        }
        if (is_past_join_screen(player)) {
            if (arguments.console) {
                send_server_notice( player, 0, "PM from Console: %s", PM);
            } else {
                send_server_notice( player, 0, "PM from %s: %s", arguments.player->name, PM);
            }
            send_server_notice(arguments.player, arguments.console, "PM sent to %s", arguments.player->name);
        }
    } else {
        send_server_notice(arguments.player, arguments.console, "No ID or invalid message");
    }
}
