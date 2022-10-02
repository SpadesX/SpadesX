#include <Server/Server.h>
#include <Util/Compress.h>
#include <Util/Log.h>

void send_map_start(server_t* server, uint8_t player_id)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    player_t* player = &server->player[player_id];
    LOG_INFO("Sending map info to %s (#%hhu)", player->name, player_id);
    ENetPacket* packet = enet_packet_create(NULL, 5, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    stream_write_u8(&stream, PACKET_TYPE_MAP_START);
    stream_write_u32(&stream, server->s_map.compressed_size);
    if (enet_peer_send(player->peer, 0, packet) == 0) {
        player->state = STATE_LOADING_CHUNKS;

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
        map                          = old_map;
        server->s_map.compressed_map = compress_queue(map, server->s_map.map_size, DEFAULT_COMPRESS_CHUNK_SIZE);
        player->queues               = server->s_map.compressed_map;
        free(map);
    }
}
