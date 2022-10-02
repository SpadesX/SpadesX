#include <Server/Server.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Notice.h>

void cmd_say(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    if (arguments.argc == 2) {
        for (uint8_t ID = 0; ID < server->protocol.max_players; ++ID) {
            if (is_past_join_screen(server, ID)) {
                send_server_notice(server, ID, 0, arguments.argv[1]);
            }
        }
    } else {
        send_server_notice(server, arguments.player_id, arguments.console, "Invalid message");
    }
}
