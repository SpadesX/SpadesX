#include <Server/Server.h>
#include <Util/Checks/PlayerChecks.h>

void send_world_update(server_t* server, uint8_t player_id)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 1 + (32 * 24), 0);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    player_t*   player = &server->player[player_id];
    stream_write_u8(&stream, PACKET_TYPE_WORLD_UPDATE);

    for (uint8_t j = 0; j < server->protocol.max_players; ++j) {
        if (player_to_player_visibile(server, player_id, j) && server->player[j].is_invisible == 0) {
            /*float    dt       = (getNanos() - server->globalTimers.lastUpdateTime) / 1000000000.f;
            Vector3f position = {server->player[j].movement.velocity.x * dt + server->player[j].movement.position.x,
                                 server->player[j].movement.velocity.y * dt + server->player[j].movement.position.y,
                                 server->player[j].movement.velocity.z * dt + server->player[j].movement.position.z};
            WriteVector3f(&stream, position);*/
            stream_write_vector3f(&stream, server->player[j].movement.position);
            stream_write_vector3f(&stream, server->player[j].movement.forward_orientation);
        } else {
            vector3f_t empty;
            empty.x = 0;
            empty.y = 0;
            empty.z = 0;
            stream_write_vector3f(&stream, empty);
            stream_write_vector3f(&stream, empty);
        }
    }
    if (enet_peer_send(player->peer, 0, packet) != 0) {
        enet_packet_destroy(packet);
    }
}
