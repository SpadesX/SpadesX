#include <Server/Packets/Packets.h>
#include <Server/Server.h>
#include <Util/Log.h>

void receive_change_team(server_t* server, uint8_t player_id, stream_t* data)
{
    uint8_t   received_id = stream_read_u8(data);
    player_t* player      = &server->player[player_id];
    server->protocol.num_team_users[player->team]--;
    uint8_t team = stream_read_u8(data);

    if (team != 0 && team != 1 && team != 255) {
        LOG_WARNING("Player %s (#%hhu) sent invalid team. Switching them to Spectator", player->name, player_id);
        team = 255;
    }

    player->team = team;
    if (player_id != received_id) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in change team packet", player_id, received_id);
    }
    server->protocol.num_team_users[player->team]++;
    send_kill_action_packet(server, player_id, player_id, 5, 5, 0);
    player->state = STATE_WAITING_FOR_RESPAWN;
}
