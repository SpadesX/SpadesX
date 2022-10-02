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
        if (ID < server->protocol.max_players && is_past_join_screen(server, ID)) {
            send_server_notice(server,
                               arguments.player_id,
                               arguments.console,
                               "%s has kill to death ratio of: %f (Kills: %d, Deaths: %d)",
                               server->player[ID].name,
                               ((float) server->player[ID].kills / fmaxf(1, (float) server->player[ID].deaths)),
                               server->player[ID].kills,
                               server->player[ID].deaths);
        } else {
            send_server_notice(
            server, arguments.player_id, arguments.console, "ID not in range or player doesnt exist");
        }
    } else {
        if (arguments.console) {
            send_server_notice(server,
                               arguments.player_id,
                               arguments.console,
                               "You cannot use this command from console without argument");
            return;
        }
        send_server_notice(server,
                           arguments.player_id,
                           arguments.console,
                           "%s has kill to death ratio of: %f (Kills: %d, Deaths: %d)",
                           server->player[arguments.player_id].name,
                           ((float) server->player[arguments.player_id].kills /
                            fmaxf(1, (float) server->player[arguments.player_id].deaths)),
                           server->player[arguments.player_id].kills,
                           server->player[arguments.player_id].deaths);
    }
}
