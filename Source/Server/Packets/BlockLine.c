#include "Util/Uthash.h"
#include <Server/IntelTent.h>
#include <Server/Server.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Checks/PositionChecks.h>
#include <Util/Checks/TimeChecks.h>
#include <Util/Log.h>
#include <Util/Nanos.h>
#include <Util/Utlist.h>
#include <pthread.h>

void send_block_line(server_t* server, player_t* player, vector3i_t start, vector3i_t end)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 26, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    stream_write_u8(&stream, PACKET_TYPE_BLOCK_LINE);
    stream_write_u8(&stream, player->id);
    stream_write_u32(&stream, start.x);
    stream_write_u32(&stream, start.y);
    stream_write_u32(&stream, start.z);
    stream_write_u32(&stream, end.x);
    stream_write_u32(&stream, end.y);
    stream_write_u32(&stream, end.z);
    uint8_t sent = 0;
    player_t *check, *tmp;
    HASH_ITER(hh, server->players, check, tmp) {
        if (is_past_state_data(check)) {
            if (enet_peer_send(player->peer, 0, packet) == 0) {
                sent = 1;
            }
        } else if (player->state == STATE_STARTING_MAP || player->state == STATE_LOADING_CHUNKS) {
            block_node_t* node   = (block_node_t*) malloc(sizeof(*node));
            node->position.x     = start.x;
            node->position.y     = start.y;
            node->position.z     = start.z;
            node->position_end.x = end.x;
            node->position_end.y = end.y;
            node->position_end.z = end.z;
            node->color          = player->color;
            node->type           = 10;
            node->sender_id      = player->id;
            LL_APPEND(player->blockBuffer, node);
        }
    }
    if (sent == 0) {
        enet_packet_destroy(packet);
    }
}

void send_block_line_to_player(server_t* server, player_t* player, player_t* receiver, vector3i_t start, vector3i_t end)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 26, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    stream_write_u8(&stream, PACKET_TYPE_BLOCK_LINE);
    stream_write_u8(&stream, player->id);
    stream_write_u32(&stream, start.x);
    stream_write_u32(&stream, start.y);
    stream_write_u32(&stream, start.z);
    stream_write_u32(&stream, end.x);
    stream_write_u32(&stream, end.y);
    stream_write_u32(&stream, end.z);
    uint8_t sent = 0;
    if (enet_peer_send(receiver->peer, 0, packet) == 0) {
        sent = 1;
    }
    if (sent == 0) {
        enet_packet_destroy(packet);
    }
}

void receive_block_line(server_t* server, player_t* player, stream_t* data)
{
    uint8_t received_id = stream_read_u8(data);
    if (player->id != received_id) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in blockline packet", player->id, received_id);
    }
    pthread_mutex_lock(&server->mutex.physics);
    uint64_t  time_now = get_nanos();
    if (player->blocks > 0 && player->can_build && server->global_ab && player->item == 1 &&
        diff_is_older_then(time_now, &player->timers.since_last_block_plac, BLOCK_DELAY) &&
        diff_is_older_then_dont_update(time_now, player->timers.since_last_block_dest, BLOCK_DELAY) &&
        diff_is_older_then_dont_update(time_now, player->timers.since_last_3block_dest, BLOCK_DELAY))
    {
        vector3i_t start;
        vector3i_t end;
        start.x = stream_read_u32(data);
        start.y = stream_read_u32(data);
        start.z = stream_read_u32(data);
        end.x   = stream_read_u32(data);
        end.y   = stream_read_u32(data);
        end.z   = stream_read_u32(data);

        if (player->sprinting) {
            return;
        }
        vector3f_t startF = {start.x, start.y, start.z};
        vector3f_t endF   = {end.x, end.y, end.z};
        if (distance_in_3d(endF, player->movement.position) <= 4 && distance_in_3d(startF, player->locAtClick) <= 4 &&
            valid_pos_v3f(server, startF) && valid_pos_v3f(server, endF))
        {
            int size = line_get_blocks(&start, &end, server->s_map.result_line);
            player->blocks -= size;
            for (int i = 0; i < size; i++) {
                mapvxl_set_color(&server->s_map.map,
                                 server->s_map.result_line[i].x,
                                 server->s_map.result_line[i].y,
                                 server->s_map.result_line[i].z,
                                 player->tool_color.raw);
            }
            moveIntelAndTentUp(server);
            send_block_line(server, player, start, end);
        }
    }
    pthread_mutex_unlock(&server->mutex.physics);
}
