#include <Server/Server.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Uthash.h>

void send_intel_capture(server_t* server, player_t* player, uint8_t winning)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    uint8_t team;
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
    stream_write_u8(&stream, player->id);
    stream_write_u8(&stream, winning);
    player->has_intel                          = 0;
    server->protocol.gamemode.intel_held[team] = 0;

    uint8_t   sent = 0;
    player_t *connected_player, *tmp;
    HASH_ITER(hh, server->players, connected_player, tmp)
    {
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
