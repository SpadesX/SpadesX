#include <Server/Structs/ServerStruct.h>
#include <Util/Checks/PlayerChecks.h>

uint8_t send_packet_except_sender(server_t* server, ENetPacket* packet, uint8_t player_id)
{
    uint8_t sent = 0;
    for (uint8_t i = 0; i < 32; ++i) {
        if (player_id != i && is_past_state_data(server, i)) {
            if (enet_peer_send(server->player[i].peer, 0, packet) == 0) {
                sent = 1;
            }
        }
    }
    return sent;
}

uint8_t send_packet_except_sender_dist_check(server_t* server, ENetPacket* packet, uint8_t player_id)
{
    uint8_t sent = 0;
    for (uint8_t i = 0; i < 32; ++i) {
        if (player_id != i && is_past_state_data(server, i)) {
            if (player_to_player_visibile(server, player_id, i) || server->player[i].team == TEAM_SPECTATOR) {
                if (enet_peer_send(server->player[i].peer, 0, packet) == 0) {
                    sent = 1;
                }
            }
        }
    }
    return sent;
}

uint8_t send_packet_dist_check(server_t* server, ENetPacket* packet, uint8_t player_id)
{
    uint8_t sent = 0;
    for (uint8_t i = 0; i < 32; ++i) {
        if (is_past_state_data(server, i)) {
            if (player_to_player_visibile(server, player_id, i) || server->player[i].team == TEAM_SPECTATOR) {
                if (enet_peer_send(server->player[i].peer, 0, packet) == 0) {
                    sent = 1;
                }
            }
        }
    }
    return sent;
}
