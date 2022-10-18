#include "Util/Uthash.h"
#include <Server/Server.h>
#include <Util/Checks/PlayerChecks.h>

void send_move_object(server_t* server, uint8_t object, uint8_t team, vector3f_t pos)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 15, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    stream_write_u8(&stream, PACKET_TYPE_MOVE_OBJECT);
    stream_write_u8(&stream, object);
    stream_write_u8(&stream, team);
    stream_write_f(&stream, pos.x);
    stream_write_f(&stream, pos.y);
    stream_write_f(&stream, pos.z);

    uint8_t sent = 0;
    player_t *connected_player, *tmp;
    HASH_ITER(hh, server->players, connected_player, tmp) {
        if (is_past_state_data(connected_player)) {
            if (enet_peer_send(connected_player->peer, 0, packet) == 0) {
                sent = 1;
            }
        }
    }
    if (sent == 0) {
        enet_packet_destroy(packet);
    }
}
