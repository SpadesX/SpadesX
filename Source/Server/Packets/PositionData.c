#include "Util/Log.h"
#include <Server/Server.h>
#include <Util/Checks/PositionChecks.h>
#include <Util/Checks/VectorChecks.h>

void send_position_packet(server_t* server, player_t* player, float x, float y, float z)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 13, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    stream_write_u8(&stream, PACKET_TYPE_POSITION_DATA);
    stream_write_f(&stream, x);
    stream_write_f(&stream, y);
    stream_write_f(&stream, z);
    if (enet_peer_send(player->peer, 0, packet) != 0) {
        enet_packet_destroy(packet);
    }
}

void receive_position_data(server_t* server, player_t* player, stream_t* data)
{
    vector3f_t position = {stream_read_f(data), stream_read_f(data), stream_read_f(data)};

    if (!valid_vec3f(position)) {
        return;
    }

    if (distance_in_3d(player->movement.position, position) >= 6) {
        send_position_packet(
        server, player, player->movement.position.x, player->movement.position.y, player->movement.position.z);
        return;
    }

    if (player->movement.position.z > 0) {
        uint16_t x = player->movement.position.x;
        uint16_t y = player->movement.position.y;
        uint16_t z = player->movement.position.z;

        if (z <= server->s_map.map.size_z - 2 && mapvxl_is_solid(&server->s_map.map, x, y, z) &&
            mapvxl_is_solid(&server->s_map.map, x, y, z + 1) && mapvxl_is_solid(&server->s_map.map, x, y, z + 2))
        {
            return;
        }
    }

    player->movement.prev_position = player->movement.position;
    player->movement.position.x    = position.x;
    player->movement.position.y    = position.y;
    player->movement.position.z    = position.z;

    if (valid_player_pos(server, player, position.x, position.y, position.z)) {
        player->movement.prev_legit_pos = player->movement.position;
    }
}
