#include <Server/Commands/Commands.h>
#include <Server/Packets/Packets.h>
#include <Server/ParseConvert.h>
#include <Server/Server.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Log.h>
#include <Util/Notice.h>

void cmd_kill(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    if (arguments.argc == 1) {
        if (arguments.console) {
            LOG_INFO("You cannot use this command without arguments from console");
            return;
        }
        send_kill_action_packet(server, arguments.player_id, arguments.player_id, 0, 5, 0);
    } else {
        if (!player_has_permission(server, arguments.player_id, arguments.console, 30)) {
            send_server_notice(
            server, arguments.player_id, arguments.console, "You have no permission to use this command.");
            return;
        }
        if (strcmp(arguments.argv[1], "all") == 0) { // KILL THEM ALL!!!! >:D
            int count = 0;
            for (int i = 0; i < server->protocol.max_players; ++i) {
                if (is_past_join_screen(server, i) && server->player[i].hp > 0 &&
                    server->player[i].team != TEAM_SPECTATOR)
                {
                    send_kill_action_packet(server, i, i, 0, 5, 0);
                    count++;
                }
            }
            send_server_notice(server, arguments.player_id, arguments.console, "Killed %i players.", count);
            return;
        }

        uint8_t ID = 0;
        for (uint32_t i = 1; i < arguments.argc; i++) {
            if (!parse_player(arguments.argv[i], &ID, NULL) || ID > 32) {
                send_server_notice(
                server, arguments.player_id, arguments.console, "Invalid player \"%s\"!", arguments.argv[i]);
                return;
            }
            if (is_past_join_screen(server, ID) && server->player[i].team != TEAM_SPECTATOR) {
                send_kill_action_packet(server, ID, ID, 0, 5, 0);
                send_server_notice(server, arguments.player_id, arguments.console, "Killing player #%i...", ID);
            } else {
                send_server_notice(
                server, arguments.player_id, arguments.console, "Player %hhu does not exist or is already dead", ID);
            }
        }
    }
}
