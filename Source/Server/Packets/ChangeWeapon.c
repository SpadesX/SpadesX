#include <Server/Packets/Packets.h>
#include <Server/Server.h>
#include <Util/Log.h>

void receive_change_weapon(server_t* server, uint8_t player_id, stream_t* data)
{
    uint8_t   received_id     = stream_read_u8(data);
    uint8_t   received_weapon = stream_read_u8(data);
    player_t* player          = &server->player[player_id];
    if (player->weapon == received_weapon) {
        return;
    }
    player->weapon = received_weapon;
    if (player_id != received_id) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in change weapon packet", player_id, received_id);
    }
    send_kill_action_packet(server, player_id, player_id, 6, 5, 0);
    player->state = STATE_WAITING_FOR_RESPAWN;
}
