#include <Server/Block.h>
#include <Server/Gamemodes/Gamemodes.h>
#include <Server/IntelTent.h>
#include <Server/Nodes.h>
#include <Server/Server.h>
#include <Server/Staff.h>
#include <Server/Structs/CommandStruct.h>
#include <Server/Structs/PacketStruct.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Checks/PositionChecks.h>
#include <Util/Checks/TimeChecks.h>
#include <Util/Enums.h>
#include <Util/Log.h>
#include <Util/Nanos.h>
#include <Util/Uthash.h>
#include <Util/Utlist.h>
#include <Util/Alloc.h>
#include <stdint.h>

void send_block_action(server_t* server, player_t* player, uint8_t actionType, int X, int Y, int Z)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 15, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    stream_write_u8(&stream, PACKET_TYPE_BLOCK_ACTION);
    stream_write_u8(&stream, player->id);
    stream_write_u8(&stream, actionType);
    stream_write_u32(&stream, X);
    stream_write_u32(&stream, Y);
    stream_write_u32(&stream, Z);
    uint8_t   sent = 0;
    player_t *check, *tmp;
    HASH_ITER(hh, server->players, check, tmp)
    {
        if (is_past_state_data(check)) {
            if (enet_peer_send(check->peer, 0, packet) == 0) {
                sent = 1;
            }
        } else if (player->state == STATE_STARTING_MAP || player->state == STATE_LOADING_CHUNKS) {
            block_node_t* node = (block_node_t*) spadesx_malloc(sizeof(*node));
            node->position.x   = X;
            node->position.y   = Y;
            node->position.z   = Z;
            node->color        = player->tool_color;
            node->type         = actionType;
            node->sender_id    = player->id;
            LL_APPEND(player->blockBuffer, node);
        }
    }
    if (sent == 0) {
        enet_packet_destroy(packet);
    }
}
void send_block_action_to_player(server_t* server,
                                 player_t* player,
                                 player_t* receiver,
                                 uint8_t   actionType,
                                 int       X,
                                 int       Y,
                                 int       Z)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 15, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    stream_write_u8(&stream, PACKET_TYPE_BLOCK_ACTION);
    stream_write_u8(&stream, player->id);
    stream_write_u8(&stream, actionType);
    stream_write_u32(&stream, X);
    stream_write_u32(&stream, Y);
    stream_write_u32(&stream, Z);
    uint8_t sent = 0;
    if (enet_peer_send(receiver->peer, 0, packet) == 0) {
        sent = 1;
    }
    if (sent == 0) {
        enet_packet_destroy(packet);
    }
}

void receive_block_action(server_t* server, player_t* player, stream_t* data)
{
    uint8_t received_id = stream_read_u8(data);
    if (player->id != received_id) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in block action packet", player->id, received_id);
    }
    if (!(player->can_build && server->global_ab) || player->sprinting) {
        return;
    }
    uint8_t    action_type   = stream_read_u8(data);
    uint32_t   X             = stream_read_u32(data);
    uint32_t   Y             = stream_read_u32(data);
    uint32_t   Z             = stream_read_u32(data);
    vector3i_t vector_block  = {X, Y, Z};
    vector3f_t vectorf_block = {(float) X, (float) Y, (float) Z};
    vector3f_t player_vector = player->movement.position;
    if (!((player->item == TOOL_SPADE && (action_type == 1 || action_type == 2)) ||
          (player->item == TOOL_BLOCK && action_type == 0) || (player->item == TOOL_GUN && action_type == 1)))
    {
        LOG_WARNING("Player %s (#%hhu) may be using BlockExploit with Item: %d and Action: %d",
                    player->name,
                    player->id,
                    player->item,
                    action_type);
        return;
    }
    handle_block_action(server, player, action_type, vector_block, vectorf_block, player_vector, X, Y, Z);
    move_intel_and_tent_down(server);
}
