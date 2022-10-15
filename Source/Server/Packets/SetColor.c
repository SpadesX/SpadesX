#include <Server/Server.h>
#include <Util/Checks/PacketChecks.h>
#include <Util/Log.h>

void send_set_color(server_t* server, uint8_t player_id, color_t color)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 5, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    stream_write_u8(&stream, PACKET_TYPE_SET_COLOR);
    stream_write_u8(&stream, player_id);
    stream_write_color_rgb(&stream, color);
    if (send_packet_except_sender(server, packet, player_id) == 0) {
        enet_packet_destroy(packet);
    }
}

void send_set_color_to_player(server_t* server, uint8_t player_id, uint8_t sendToID, color_t color)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 5, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    stream_write_u8(&stream, PACKET_TYPE_SET_COLOR);
    stream_write_u8(&stream, player_id);
    stream_write_color_rgb(&stream, color);
    if (enet_peer_send(server->player[sendToID].peer, 0, packet) != 0) {
        enet_packet_destroy(packet);
    }
}

void receive_set_color(server_t* server, uint8_t player_id, stream_t* data)
{
    uint8_t received_id    = stream_read_u8(data);
    color_t received_color = stream_read_color_rgb(data);

    if (player_id != received_id) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in set color packet", player_id, received_id);
    }

    player_t* player   = &server->player[player_id];
    player->tool_color = received_color;
    send_set_color(server, player_id, received_color);
}
