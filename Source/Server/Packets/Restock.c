#include <Server/Server.h>

void send_restock(server_t* server, uint8_t player_id)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 2, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    player_t*   player = &server->player[player_id];
    stream_write_u8(&stream, PACKET_TYPE_RESTOCK);
    stream_write_u8(&stream, player_id);
    if (enet_peer_send(player->peer, 0, packet) != 0) {
        enet_packet_destroy(packet);
    }
}
