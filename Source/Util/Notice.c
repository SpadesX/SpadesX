#include <Server/Structs/ServerStruct.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Log.h>
#include <Util/Uthash.h>
#include <stdarg.h>
#include <stdio.h>

void send_server_notice(player_t* player, uint8_t console, const char* message, ...)
{
    va_list args;
    va_start(args, message);
    char fMessage[1024];
    vsnprintf(fMessage, 1024, message, args);
    va_end(args);

    if (console) {
        LOG_INFO("%s", fMessage);
        return;
    }

    uint8_t     utf8         = player->client == 'o';
    uint32_t    fMessageSize = strlen(fMessage);
    uint32_t    packetSize   = 3 + fMessageSize + utf8; // add 1 space for the magic byte
    ENetPacket* packet       = enet_packet_create(NULL, packetSize, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream       = {packet->data, packet->dataLength, 0};
    stream_write_u8(&stream, PACKET_TYPE_CHAT_MESSAGE);
    stream_write_u8(&stream, player->id);
    stream_write_u8(&stream, 2);
    if(utf8) {
        stream_write_u8(&stream, 0xFF);
    }

    stream_write_array(&stream, fMessage, fMessageSize);
    if (enet_peer_send(player->peer, 0, packet) != 0) {
        enet_packet_destroy(packet);
    }
}

void broadcast_server_notice(server_t* server, uint8_t console, const char* message, ...)
{
    va_list args;
    va_start(args, message);
    char fMessage[1024];
    vsnprintf(fMessage, 1024, message, args);
    va_end(args);

    uint32_t    fMessageSize = strlen(fMessage);
    uint32_t    packetSize   = 3 + fMessageSize;
    ENetPacket* packet       = enet_packet_create(NULL, packetSize, ENET_PACKET_FLAG_RELIABLE);
    ENetPacket* packet_utf8  = enet_packet_create(NULL, packetSize + 1, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream       = {packet->data, packet->dataLength, 0};
    stream_t    stream_utf8  = {packet_utf8->data, packet_utf8->dataLength, 0};

    stream_write_u8(&stream, PACKET_TYPE_CHAT_MESSAGE);
    stream_write_u8(&stream, 33);
    stream_write_u8(&stream, 2);
    stream_write_array(&stream, fMessage, fMessageSize);

    stream_write_u8(&stream_utf8, PACKET_TYPE_CHAT_MESSAGE);
    stream_write_u8(&stream_utf8, 33);
    stream_write_u8(&stream_utf8, 2);
    stream_write_u8(&stream_utf8, 0xFF);
    stream_write_array(&stream_utf8, fMessage, fMessageSize);

    if (console) {
        LOG_INFO("%s", fMessage);
    }
    player_t *player, *tmp;
    uint8_t   sent = 0;
    HASH_ITER(hh, server->players, player, tmp)
    {
        if (is_past_join_screen(player)) {
            if(player->client == 'o') {
                if (enet_peer_send(player->peer, 0, packet_utf8) == 0) {
                    sent = 1;
                }
            } else {
                if (enet_peer_send(player->peer, 0, packet) == 0) {
                    sent = 1;
                }
            }
        }
    }
    if (sent == 0) {
        enet_packet_destroy(packet);
    }
}
