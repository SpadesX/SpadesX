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
        if (ID < server->protocol.max_players && is_past_join_screen(server, ID)) {
            if (arguments.console) {
                send_server_notice(server, ID, 0, "PM from Console: %s", PM);
            } else {
                send_server_notice(server, ID, 0, "PM from %s: %s", server->player[arguments.player_id].name, PM);
            }
            send_server_notice(
            server, arguments.player_id, arguments.console, "PM sent to %s", server->player[ID].name);
        } else {
            send_server_notice(server, arguments.player_id, arguments.console, "Invalid ID. Player doesnt exist");
        }
    } else {
        send_server_notice(server, arguments.player_id, arguments.console, "No ID or invalid message");
    }
}
