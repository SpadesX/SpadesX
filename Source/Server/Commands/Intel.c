#include <Server/Server.h>
#include <Util/Notice.h>

void cmd_intel(void* p_server, command_args_t arguments)
{
    server_t* server          = (server_t*) p_server;
    uint8_t   sentAtLeastOnce = 0;
    for (uint8_t player_id = 0; player_id < server->protocol.max_players; ++player_id) {
        if (server->player[player_id].state != STATE_DISCONNECTED && server->player[player_id].has_intel) {
            send_server_notice(server,
                               arguments.player_id,
                               arguments.console,
                               "Player %s (#%hhu) has intel",
                               server->player[player_id].name,
                               player_id);
            sentAtLeastOnce = 1;
        }
    }
    if (sentAtLeastOnce == 0) {
        if (server->protocol.gamemode.intel_held[0]) {
            send_server_notice(
            server, arguments.player_id, arguments.console, "Intel is not being held but intel of team 0 thinks it is");
        } else if (server->protocol.gamemode.intel_held[1]) {
            send_server_notice(
            server, arguments.player_id, arguments.console, "Intel is not being held but intel of team 1 thinks it is");
        }
        send_server_notice(server, arguments.player_id, arguments.console, "Intel is not being held");
    }
}
