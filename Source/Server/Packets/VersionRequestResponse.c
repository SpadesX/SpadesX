#include <Server/Server.h>
#include <stdio.h>

void send_version_request(server_t* server, player_t* player)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 1, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    stream_write_u8(&stream, PACKET_TYPE_VERSION_REQUEST);
    if (enet_peer_send(player->peer, 0, packet) != 0) {
        enet_packet_destroy(packet);
    }
}

void receive_version_response(server_t* server, player_t* player, stream_t* data)
{
    (void) server;
    player->client           = stream_read_u8(data);
    player->version_major    = stream_read_u8(data);
    player->version_minor    = stream_read_u8(data);
    player->version_revision = stream_read_u8(data);
    uint32_t length          = stream_left(data);
    if (length < 255) {
        player->os_info[length] = '\0';
        stream_read_array(data, player->os_info, length);
    } else {
        snprintf(player->os_info, 8, "Unknown");
    }
    if (player->client == 'o') {
        if (!(player->version_major == 0 && player->version_minor == 1 &&
              (player->version_revision == 3 || player->version_revision == 5)))
        {
            enet_peer_disconnect(player->peer, REASON_KICKED);
        }
    } else if (player->client == 'B') {
        if (!(player->version_major == 0 && player->version_minor == 1 && player->version_revision == 5)) {
            enet_peer_disconnect(player->peer, REASON_KICKED);
        }
    }
}
