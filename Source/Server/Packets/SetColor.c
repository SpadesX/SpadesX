#include "Util/Uthash.h"
#include <Server/Server.h>
#include <Util/Checks/PacketChecks.h>
#include <Util/Log.h>

void send_set_color(server_t* server, player_t* player, color_t color)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 5, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    stream_write_u8(&stream, PACKET_TYPE_SET_COLOR);
    stream_write_u8(&stream, player->id);
    stream_write_color_rgb(&stream, color);
    if (send_packet_except_sender(server, packet, player) == 0) {
        enet_packet_destroy(packet);
    }
}

void send_set_color_to_player(server_t* server, player_t* player, player_t* receiver, color_t color)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 5, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    stream_write_u8(&stream, PACKET_TYPE_SET_COLOR);
    stream_write_u8(&stream, player->id);
    stream_write_color_rgb(&stream, color);
    if (enet_peer_send(receiver->peer, 0, packet) != 0) {
        enet_packet_destroy(packet);
    }
}

void receive_set_color(server_t* server, player_t* player, stream_t* data)
{
    uint8_t received_id    = stream_read_u8(data);
    color_t received_color = stream_read_color_rgb(data);

    if (player->id != received_id) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in set color packet", player->id, received_id);
    }

    player->tool_color = received_color;
    send_set_color(server, player, received_color);
}
