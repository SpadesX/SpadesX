#include <Server/Server.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Log.h>
#include <Util/Nanos.h>

void send_weapon_reload(server_t* server, uint8_t player_id, uint8_t startAnimation, uint8_t clip, uint8_t reserve)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 4, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    player_t*   player = &server->player[player_id];
    stream_write_u8(&stream, PACKET_TYPE_WEAPON_RELOAD);
    stream_write_u8(&stream, player_id);
    if (startAnimation) {
        stream_write_u8(&stream, clip);
        stream_write_u8(&stream, reserve);
    } else {
        stream_write_u8(&stream, player->weapon_clip);
        stream_write_u8(&stream, player->weapon_reserve);
    }
    if (startAnimation) {
        uint8_t sendSucc = 0;
        for (int player = 0; player < server->protocol.max_players; ++player) {
            if (is_past_state_data(server, player) && player != player_id) {
                if (enet_peer_send(server->player[player].peer, 0, packet) == 0) {
                    sendSucc = 1;
                }
            }
        }
        if (sendSucc == 0) {
            enet_packet_destroy(packet);
        }
    } else {
        if (is_past_join_screen(server, player_id)) {
            if (enet_peer_send(player->peer, 0, packet) != 0) {
                enet_packet_destroy(packet);
            }
        }
    }
}

void receive_weapon_reload(server_t* server, uint8_t player_id, stream_t* data)
{
    uint8_t ID      = stream_read_u8(data);
    uint8_t clip    = stream_read_u8(data);
    uint8_t reserve = stream_read_u8(data);
    if (player_id != ID) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in weapon reload packet", player_id, ID);
    }
    player_t* player       = &server->player[player_id];
    player->primary_fire   = 0;
    player->secondary_fire = 0;

    if (player->weapon_reserve == 0 || player->reloading) {
        return;
    }
    player->reloading                 = 1;
    player->timers.since_reload_start = get_nanos();
    send_weapon_reload(server, player_id, 1, clip, reserve);
}
