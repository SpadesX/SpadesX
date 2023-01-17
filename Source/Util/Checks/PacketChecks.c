#include <Server/Structs/ServerStruct.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Uthash.h>

uint8_t send_packet_except_sender(server_t* server, ENetPacket* packet, player_t* sender)
{
    uint8_t   sent = 0;
    player_t *player, *tmp;
    HASH_ITER(hh, server->players, player, tmp)
    {
        if (sender->id != player->id && is_past_state_data(player)) {
            if (enet_peer_send(player->peer, 0, packet) == 0) {
                sent = 1;
            }
        }
    }
    return sent;
}

uint8_t send_packet_except_sender_dist_check(server_t* server, ENetPacket* packet, player_t* sender)
{
    uint8_t   sent = 0;
    player_t *player, *tmp;
    HASH_ITER(hh, server->players, player, tmp)
    {
        if (sender->id != player->id && is_past_state_data(player)) {
            if (player_to_player_visibile(sender, player) || player->team == TEAM_SPECTATOR) {
                if (enet_peer_send(player->peer, 0, packet) == 0) {
                    sent = 1;
                }
            }
        }
    }
    return sent;
}

uint8_t send_packet_dist_check(server_t* server, ENetPacket* packet, player_t* player)
{
    uint8_t   sent = 0;
    player_t *receiver, *tmp;
    HASH_ITER(hh, server->players, receiver, tmp)
    {
        if (is_past_state_data(receiver)) {
            if (player_to_player_visibile(player, receiver) || receiver->team == TEAM_SPECTATOR) {
                if (enet_peer_send(receiver->peer, 0, packet) == 0) {
                    sent = 1;
                }
            }
        }
    }
    return sent;
}
