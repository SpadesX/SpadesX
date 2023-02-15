#include "Util/Utlist.h"

#include <Server/Packets/Packets.h>
#include <Server/Server.h>
#include <Util/Log.h>

void send_map_chunks(server_t* server, player_t* player)
{
    queue_t *node, *tmp;
    DL_FOREACH_SAFE(player->map_queue, node, tmp)
    {
        ENetPacket* packet = enet_packet_create(NULL, player->map_queue->length + 1, ENET_PACKET_FLAG_RELIABLE);
        stream_t    stream = {packet->data, packet->dataLength, 0};
        stream_write_u8(&stream, PACKET_TYPE_MAP_CHUNK);
        stream_write_array(&stream, player->map_queue->block, player->map_queue->length);
        enet_peer_send(player->peer, 0, packet);
        free(node->block);
        DL_DELETE(player->map_queue, node);
        free(node);
    }
    player->map_queue = NULL;
    send_version_request(server, player);
    player->state = STATE_JOINING;
    LOG_INFO("Finished sending map chunks to %s (#%hhu)", player->name, player->id);
}
