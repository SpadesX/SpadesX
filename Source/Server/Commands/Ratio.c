#include <Server/ParseConvert.h>
#include <Server/Server.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Notice.h>
#include <math.h>

void cmd_ratio(void* p_server, command_args_t arguments)
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
            send_server_notice(
                               arguments.player,
                               arguments.console,
                               "%s has kill to death ratio of: %f (Kills: %d, Deaths: %d)",
                               player->name,
                               ((float) player->kills / fmaxf(1, (float) player->deaths)),
                               player->kills,
                               player->deaths);
        }
    } else {
        if (arguments.console) {
            send_server_notice(
                               arguments.player,
                               arguments.console,
                               "You cannot use this command from console without argument");
            return;
        }
        send_server_notice(
                           arguments.player,
                           arguments.console,
                           "%s has kill to death ratio of: %f (Kills: %d, Deaths: %d)",
                           arguments.player->name,
                           ((float) arguments.player->kills /
                            fmaxf(1, (float) arguments.player->deaths)),
                           arguments.player->kills,
                           arguments.player->deaths);
    }
}
