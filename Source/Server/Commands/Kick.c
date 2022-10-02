#include <Server/ParseConvert.h>
#include <Server/Server.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Notice.h>

void cmd_kick(void* p_server, command_args_t arguments)
{
    server_t* server = (server_t*) p_server;
    uint8_t   ID     = 33;
    if (arguments.argc == 2 && parse_player(arguments.argv[1], &ID, NULL)) {
        if (ID < server->protocol.max_players && is_past_join_screen(server, ID)) {
            enet_peer_disconnect(server->player[ID].peer, REASON_KICKED);
            broadcast_server_notice(
            server, arguments.console, "Player %s (#%hhu) has been kicked", server->player[ID].name, ID);
        } else {
            send_server_notice(
            server, arguments.player_id, arguments.console, "ID not in range or player doesnt exist");
        }
    } else {
        send_server_notice(
        server, arguments.player_id, arguments.console, "You did not enter ID or entered incorrect argument");
    }
}
