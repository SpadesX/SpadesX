#include "Util/Enums.h"
#include "Util/Uthash.h"

#include <Server/Server.h>
#include <Util/Checks/PlayerChecks.h>

void send_world_update(server_t* server, player_t* player)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 1 + (32 * 24), 0);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    stream_write_u8(&stream, PACKET_TYPE_WORLD_UPDATE);

    player_t *connected_player;
    for (uint8_t player_id = 0; player_id < server->protocol.max_players; ++player_id)
    {
        HASH_FIND(hh, server->players, &player_id, sizeof(player_id), connected_player);
        if ((connected_player != NULL && connected_player->state != STATE_DISCONNECTED) &&
            player_to_player_visibile(player, connected_player) && connected_player->is_invisible == 0)
        {
            stream_write_vector3f(&stream, connected_player->movement.position);
            stream_write_vector3f(&stream, connected_player->movement.forward_orientation);
        } else {
            vector3f_t empty = {0};
            stream_write_vector3f(&stream, empty);
            stream_write_vector3f(&stream, empty);
        }
    }
    if (enet_peer_send(player->peer, 0, packet) != 0) {
        enet_packet_destroy(packet);
    }
}
