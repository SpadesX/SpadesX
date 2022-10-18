#include <Server/Server.h>

void send_state_data(server_t* server, player_t* player)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 104, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    stream_write_u8(&stream, PACKET_TYPE_STATE_DATA);
    stream_write_u8(&stream, player->id);
    stream_write_color_rgb(&stream, server->protocol.color_fog);
    stream_write_color_rgb(&stream, server->protocol.color_team[0]);
    stream_write_color_rgb(&stream, server->protocol.color_team[1]);
    stream_write_array(&stream, server->protocol.name_team[0], 10);
    stream_write_array(&stream, server->protocol.name_team[1], 10);
    if (server->protocol.current_gamemode == 0 || server->protocol.current_gamemode == 1) {
        stream_write_u8(&stream, server->protocol.current_gamemode);
    } else {
        stream_write_u8(&stream, 0);
    }

    // MODE CTF:

    stream_write_u8(&stream, server->protocol.gamemode.score[0]);    // SCORE TEAM A
    stream_write_u8(&stream, server->protocol.gamemode.score[1]);    // SCORE TEAM B
    stream_write_u8(&stream, server->protocol.gamemode.score_limit); // SCORE LIMIT

    server->protocol.gamemode.intel_flags = 0;

    if (server->protocol.gamemode.intel_held[0]) {
        server->protocol.gamemode.intel_flags = INTEL_TEAM_B;
    } else if (server->protocol.gamemode.intel_held[1]) {
        server->protocol.gamemode.intel_flags = INTEL_TEAM_A;
    } else if (server->protocol.gamemode.intel_held[0] && server->protocol.gamemode.intel_held[1]) {
        server->protocol.gamemode.intel_flags = INTEL_TEAM_BOTH;
    }

    stream_write_u8(&stream, server->protocol.gamemode.intel_flags); // INTEL FLAGS

    if ((server->protocol.gamemode.intel_flags & 1) == 0) {
        stream_write_u8(&stream, server->protocol.gamemode.player_intel_team[1]);
        for (int i = 0; i < 11; ++i) {
            stream_write_u8(&stream, 255);
        }
    } else {
        stream_write_vector3f(&stream, server->protocol.gamemode.intel[0]);
    }

    if ((server->protocol.gamemode.intel_flags & 2) == 0) {
        stream_write_u8(&stream, server->protocol.gamemode.player_intel_team[0]);
        for (int i = 0; i < 11; ++i) {
            stream_write_u8(&stream, 255);
        }
    } else {
        stream_write_vector3f(&stream, server->protocol.gamemode.intel[1]);
    }

    stream_write_vector3f(&stream, server->protocol.gamemode.base[0]);
    stream_write_vector3f(&stream, server->protocol.gamemode.base[1]);

    if (enet_peer_send(player->peer, 0, packet) == 0) {
        player->state = STATE_PICK_SCREEN;
    } else {
        enet_packet_destroy(packet);
    }
}
