#include <Server/Packets/Packets.h>
#include <Server/Server.h>
#include <Util/Log.h>

void receive_change_weapon(server_t* server, player_t* player, stream_t* data)
{
    uint8_t received_id     = stream_read_u8(data);
    uint8_t received_weapon = stream_read_u8(data);
    if (player->weapon == received_weapon) {
        return;
    }
    player->weapon = received_weapon;
    if (player->id != received_id) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in change weapon packet", player->id, received_id);
    }
    send_kill_action_packet(server, player, player, 6, 5, 0);
    player->state = STATE_WAITING_FOR_RESPAWN;
}
