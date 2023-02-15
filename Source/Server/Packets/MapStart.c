#include "Util/Utlist.h"
#include <Server/Server.h>
#include <Util/Compress.h>
#include <Util/Log.h>

void send_map_start(server_t* server, player_t* player)
{
    if (server->protocol.num_players == 0) {
        return;
    }

    // The biggest possible VXL size given the XYZ size
    uint8_t* map = (uint8_t*) calloc(
    server->s_map.map.size_x * server->s_map.map.size_y * (server->s_map.map.size_z / 2), sizeof(uint8_t));
    // Write map to out
    server->s_map.map_size = mapvxl_write(&server->s_map.map, map);
    // Resize the map to the exact VXL memory size for given XYZ coordinate size
    uint8_t* old_map;
    old_map = (uint8_t*) realloc(map, server->s_map.map_size);
    if (!old_map) {
        free(map);
        return;
    }
    map               = old_map;
    player->map_queue = compress_queue(map, server->s_map.map_size, DEFAULT_COMPRESS_CHUNK_SIZE);
    free(map);

    uint32_t compressed_map_size = 0;
    queue_t* node;
    DL_FOREACH(player->map_queue, node) {
        compressed_map_size += node->length;
    }

    LOG_INFO("Sending map info to %s (#%hhu)", player->name, player->id);
    ENetPacket* packet = enet_packet_create(NULL, 5, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    stream_write_u8(&stream, PACKET_TYPE_MAP_START);
    stream_write_u32(&stream, compressed_map_size);
    if (enet_peer_send(player->peer, 0, packet) == 0) {
        player->state = STATE_LOADING_CHUNKS;
        LOG_INFO("Sending map chunks to %s (#%hhu)", player->name, player->id);
    }
}
