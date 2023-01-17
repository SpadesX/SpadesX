#include <Server/Commands/Commands.h>
#include <Server/Packets/Packets.h>
#include <Server/ParseConvert.h>
#include <Server/Server.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Log.h>
#include <Util/Notice.h>
#include <Util/Uthash.h>

void cmd_kill(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    if (arguments.argc == 1) {
        if (arguments.console) {
            LOG_INFO("You cannot use this command without arguments from console");
            return;
        }
        send_kill_action_packet(server, arguments.player, arguments.player, 0, 5, 0);
    } else {
        if (!player_has_permission(arguments.player, arguments.console, 30)) {
            send_server_notice(arguments.player, arguments.console, "You have no permission to use this command.");
            return;
        }
        if (strcmp(arguments.argv[1], "all") == 0) { // KILL THEM ALL!!!! >:D
            int       count = 0;
            player_t *connected_player, *tmp;
            HASH_ITER(hh, server->players, connected_player, tmp)
            {
                if (is_past_join_screen(connected_player) && connected_player->hp > 0 &&
                    connected_player->team != TEAM_SPECTATOR)
                {
                    send_kill_action_packet(server, connected_player, connected_player, 0, 5, 0);
                    count++;
                }
            }
            send_server_notice(arguments.player, arguments.console, "Killed %i players.", count);
            return;
        }

        uint8_t ID = 0;
        for (uint32_t i = 1; i < arguments.argc; i++) {
            if (!parse_player(arguments.argv[i], &ID, NULL) || ID > server->protocol.max_players) {
                send_server_notice(arguments.player, arguments.console, "Invalid player \"%s\"!", arguments.argv[i]);
                return;
            }
            player_t* player;
            HASH_FIND(hh, server->players, &ID, sizeof(ID), player);
            if (player == NULL) {
                send_server_notice(
                arguments.player, arguments.console, "Player %hhu does not exist or is already dead", ID);
                return;
            }
            if (is_past_join_screen(player) && player->team != TEAM_SPECTATOR) {
                send_kill_action_packet(server, player, player, 0, 5, 0);
                send_server_notice(arguments.player, arguments.console, "Killing player #%i...", ID);
            }
        }
    }
}
