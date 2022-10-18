#include "Util/Uthash.h"
#include <Server/Packets/Packets.h>
#include <Server/Server.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Log.h>

void send_intel_drop(server_t* server, player_t* player)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    uint8_t   team;
    if (player->team == 0) {
        team = 1;
    } else {
        team = 0;
    }
    if (player->has_intel == 0 || server->protocol.gamemode.intel_held[team] == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 14, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    stream_write_u8(&stream, PACKET_TYPE_INTEL_DROP);
    stream_write_u8(&stream, player->id);
    if (server->protocol.current_gamemode == GAME_MODE_BABEL) {
        stream_write_f(&stream, (float) server->s_map.map.size_x / 2);
        stream_write_f(&stream, (float) server->s_map.map.size_y / 2);
        stream_write_f(
        &stream,
        (float) mapvxl_find_top_block(&server->s_map.map, server->s_map.map.size_x / 2, server->s_map.map.size_y / 2));

        server->protocol.gamemode.intel[team].x = (float) server->s_map.map.size_x / 2;
        server->protocol.gamemode.intel[team].y = (float) server->s_map.map.size_y / 2;
        server->protocol.gamemode.intel[team].z =
        mapvxl_find_top_block(&server->s_map.map, server->s_map.map.size_x / 2, server->s_map.map.size_y / 2);
        server->protocol.gamemode.intel[player->team] = server->protocol.gamemode.intel[team];
        send_move_object(server, player->team, player->team, server->protocol.gamemode.intel[team]);
    } else {
        stream_write_f(&stream, player->movement.position.x);
        stream_write_f(&stream, player->movement.position.y);
        stream_write_f(
        &stream,
        (float) mapvxl_find_top_block(&server->s_map.map, player->movement.position.x, player->movement.position.y));

        server->protocol.gamemode.intel[team].x = (int) player->movement.position.x;
        server->protocol.gamemode.intel[team].y = (int) player->movement.position.y;
        server->protocol.gamemode.intel[team].z =
        mapvxl_find_top_block(&server->s_map.map, player->movement.position.x, player->movement.position.y);
    }
    player->has_intel                                         = 0;
    server->protocol.gamemode.player_intel_team[player->team] = 32;
    server->protocol.gamemode.intel_held[team]                = 0;

    LOG_INFO("Dropping intel at X: %d, Y: %d, Z: %d",
             (int) server->protocol.gamemode.intel[team].x,
             (int) server->protocol.gamemode.intel[team].y,
             (int) server->protocol.gamemode.intel[team].z);
    uint8_t sent = 0;
    player_t *connected_player, *tmp;
    HASH_ITER(hh, server->players, connected_player, tmp) {
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
