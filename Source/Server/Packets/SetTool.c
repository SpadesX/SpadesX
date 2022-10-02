#include <Server/Server.h>
#include <Util/Checks/PacketChecks.h>
#include <Util/Log.h>

void send_set_tool(server_t* server, uint8_t player_id, uint8_t tool)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 3, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    stream_write_u8(&stream, PACKET_TYPE_SET_TOOL);
    stream_write_u8(&stream, player_id);
    stream_write_u8(&stream, tool);
    if (send_packet_except_sender(server, packet, player_id) == 0) {
        enet_packet_destroy(packet);
    }
}

void receive_set_tool(server_t* server, uint8_t player_id, stream_t* data)
{
    uint8_t   received_id = stream_read_u8(data);
    uint8_t   tool        = stream_read_u8(data);
    player_t* player      = &server->player[player_id];
    if (player->item == tool) {
        return;
    }
    if (player_id != received_id) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in set tool packet", player_id, received_id);
    }

    player->item      = tool;
    player->reloading = 0;
    send_set_tool(server, player_id, tool);
}
