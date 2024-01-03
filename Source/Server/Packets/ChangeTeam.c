#include <Server/Packets/Packets.h>
#include <Server/Server.h>
#include <Util/Log.h>

void receive_change_team(server_t* server, player_t* player, stream_t* data)
{
    // @Todo: if player is not initialised yet (pyspades uses if not self.name) ignore this

    uint8_t received_id = stream_read_u8(data);

    uint8_t team     = stream_read_u8(data);
    uint8_t old_team = player->team;

    if (old_team == team)
        return;

    if (team != TEAM_A && team != TEAM_B && team != TEAM_SPECTATOR) {
        LOG_WARNING("Player %s (#%hhu) sent invalid team. Switching them to Spectator", player->name, player->id);
        team = TEAM_SPECTATOR;
    }

    if (player->id != received_id) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in change team packet", player->id, received_id);
    }

    if (server->protocol.num_team_users[old_team] > 0)
        server->protocol.num_team_users[old_team]--;

    player->team = team;
    server->protocol.num_team_users[team]++;

    if (old_team == TEAM_SPECTATOR) {
        send_respawn(server, player);
    } else {
        send_kill_action_packet(server, player, player, 5, 5, 0);
    }
}
