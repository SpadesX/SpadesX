#include "Util/Utlist.h"
#include <Server/Packets/Packets.h>
#include <Server/Server.h>
#include <Util/Log.h>

void send_map_chunks(server_t* server, player_t* player)
{
    if (player->queues == NULL) {
        queue_t *node, *tmp;
        DL_FOREACH_SAFE(server->s_map.compressed_map, node, tmp) {
            free(node->block);
            DL_DELETE(server->s_map.compressed_map, node);
            free(node);
        }
        send_version_request(server, player);
        player->state = STATE_JOINING;
        LOG_INFO("Finished sending map chunks to %s (#%hhu)", player->name, player->id);
    } else {
        ENetPacket* packet = enet_packet_create(NULL, player->queues->length + 1, ENET_PACKET_FLAG_RELIABLE);
        stream_t    stream = {packet->data, packet->dataLength, 0};
        stream_write_u8(&stream, PACKET_TYPE_MAP_CHUNK);
        stream_write_array(&stream, player->queues->block, player->queues->length);

        if (enet_peer_send(player->peer, 0, packet) == 0) {
            player->queues = player->queues->next;
        }
    }
}
