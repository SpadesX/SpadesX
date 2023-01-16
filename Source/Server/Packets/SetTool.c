#include "Util/Enums.h"

#include <Server/Packets/Packets.h>
#include <Server/Server.h>
#include <Util/Checks/PacketChecks.h>
#include <Util/Log.h>

void send_set_tool(server_t* server, player_t* player, uint8_t tool)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 3, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    stream_write_u8(&stream, PACKET_TYPE_SET_TOOL);
    stream_write_u8(&stream, player->id);
    stream_write_u8(&stream, tool);
    if (send_packet_except_sender(server, packet, player) == 0) {
        enet_packet_destroy(packet);
    }
}

void receive_set_tool(server_t* server, player_t* player, stream_t* data)
{
    uint8_t received_id = stream_read_u8(data);
    uint8_t tool        = stream_read_u8(data);
    if (player->item == tool) {
        return;
    }
    if (player->id != received_id) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in set tool packet", player->id, received_id);
    }

    if (player->reloading && player->item == TOOL_GUN) {
        send_weapon_reload(server, player, 0, 0, 0);
        player->reloading = 0;
    }
    player->item = tool;
    send_set_tool(server, player, tool);
}
