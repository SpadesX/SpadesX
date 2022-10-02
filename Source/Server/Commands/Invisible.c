#include <Server/Packets/Packets.h>
#include <Server/Server.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Log.h>
#include <Util/Notice.h>

void cmd_inv(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    if (arguments.console) {
        LOG_INFO("You cannot use this command from console");
        return;
    }
    if (server->player[arguments.player_id].is_invisible == 1) {
        server->player[arguments.player_id].is_invisible  = 0;
        server->player[arguments.player_id].allow_killing = 1;
        for (uint8_t i = 0; i < server->protocol.max_players; ++i) {
            if (is_past_join_screen(server, i) && i != arguments.player_id) {
                send_create_player(server, i, arguments.player_id);
            }
        }
        send_server_notice(server, arguments.player_id, arguments.console, "You are no longer invisible");
    } else if (server->player[arguments.player_id].is_invisible == 0) {
        server->player[arguments.player_id].is_invisible  = 1;
        server->player[arguments.player_id].allow_killing = 0;
        send_kill_action_packet(server, arguments.player_id, arguments.player_id, 0, 0, 1);
        send_server_notice(server, arguments.player_id, arguments.console, "You are now invisible");
    }
}
