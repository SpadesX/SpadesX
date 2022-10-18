#include <Server/Server.h>
#include <Util/Checks/PlayerChecks.h>

void send_intel_pickup(server_t* server, player_t* player)
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
    if (player->has_intel == 1 || server->protocol.gamemode.intel_held[team] == 1) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 2, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    stream_write_u8(&stream, PACKET_TYPE_INTEL_PICKUP);
    stream_write_u8(&stream, player->id);
    player->has_intel                                         = 1;
    server->protocol.gamemode.player_intel_team[player->team] = player->id;
    server->protocol.gamemode.intel_held[team]                = 1;

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
