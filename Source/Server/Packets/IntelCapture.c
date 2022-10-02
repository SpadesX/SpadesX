#include <Server/Server.h>
#include <Util/Checks/PlayerChecks.h>

void send_intel_capture(server_t* server, uint8_t player_id, uint8_t winning)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    uint8_t   team;
    player_t* player = &server->player[player_id];
    if (player->team == 0) {
        team = 1;
    } else {
        team = 0;
    }
    if (player->has_intel == 0 || server->protocol.gamemode.intel_held[team] == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 3, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    stream_write_u8(&stream, PACKET_TYPE_INTEL_CAPTURE);
    stream_write_u8(&stream, player_id);
    stream_write_u8(&stream, winning);
    player->has_intel                          = 0;
    server->protocol.gamemode.intel_held[team] = 0;

    uint8_t sent = 0;
    for (int player = 0; player < server->protocol.max_players; ++player) {
        if (is_past_state_data(server, player)) {
            if (enet_peer_send(server->player[player].peer, 0, packet) == 0) {
                sent = 1;
            }
        }
    }
    if (sent == 0) {
        enet_packet_destroy(packet);
    }
}
