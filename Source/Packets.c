// Copyright DarkNeutrino 2021
#include "../Extern/libmapvxl/libmapvxl.h"
#include "Commands.h"
#include "ParseConvert.h"
#include "Protocol.h"
#include "Structs.h"
#include "Uthash.h"
#include "Util/Compress.h"
#include "Util/DataStream.h"
#include "Util/Enums.h"
#include "Util/Line.h"
#include "Util/Log.h"
#include "Util/Physics.h"
#include "Util/Queue.h"
#include "Util/Types.h"
#include "Util/Utlist.h"

#include <ctype.h>
#include <enet/enet.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
    #define strlcat(dst, src, siz) strcat_s(dst, siz, src)
#else
    #include <bsd/string.h>
#endif

inline static uint8_t allowShot(server_t*  server,
                                uint8_t    playerID,
                                uint8_t    hitPlayerID,
                                uint64_t   timeNow,
                                float      distance,
                                long*      x,
                                long*      y,
                                long*      z,
                                vector3f_t shotPos,
                                vector3f_t shotOrien,
                                vector3f_t hitPos,
                                vector3f_t shotEyePos)
{
    uint8_t ret = 0;
    if (server->player[playerID].primary_fire &&
        ((server->player[playerID].item == 0 &&
          diffIsOlderThen(timeNow, &server->player[playerID].timers.since_last_shot, NANO_IN_MILLI * 100)) ||
         (server->player[playerID].item == 2 && server->player[playerID].weapon == 0 &&
          diffIsOlderThen(timeNow, &server->player[playerID].timers.since_last_shot, NANO_IN_MILLI * 500)) ||
         (server->player[playerID].item == 2 && server->player[playerID].weapon == 1 &&
          diffIsOlderThen(timeNow, &server->player[playerID].timers.since_last_shot, NANO_IN_MILLI * 110)) ||
         (server->player[playerID].item == 2 && server->player[playerID].weapon == 2 &&
          diffIsOlderThen(timeNow, &server->player[playerID].timers.since_last_shot, NANO_IN_MILLI * 1000))) &&
        server->player[playerID].alive && server->player[hitPlayerID].alive &&
        (server->player[playerID].team != server->player[hitPlayerID].team ||
         server->player[playerID].allow_team_killing) &&
        (server->player[playerID].allow_killing && server->global_ak) &&
        physics_validate_hit(shotPos, shotOrien, hitPos, 5) &&
        (physics_cast_ray(
         server, shotEyePos.x, shotEyePos.y, shotEyePos.z, shotOrien.x, shotOrien.y, shotOrien.z, distance, x, y, z) ==
         0 ||
         physics_cast_ray(server,
                          shotEyePos.x,
                          shotEyePos.y,
                          shotEyePos.z - 1,
                          shotOrien.x,
                          shotOrien.y,
                          shotOrien.z,
                          distance,
                          x,
                          y,
                          z) == 0))
    {
        ret = 1;
    }
    return ret;
}

void send_restock(server_t* server, uint8_t playerID)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket*  packet = enet_packet_create(NULL, 2, ENET_PACKET_FLAG_RELIABLE);
    datastream_t stream = {packet->data, packet->dataLength, 0};
    datastream_write_u8(&stream, PACKET_TYPE_RESTOCK);
    datastream_write_u8(&stream, playerID);
    if (enet_peer_send(server->player[playerID].peer, 0, packet) != 0) {
        enet_packet_destroy(packet);
    }
}

void send_move_object(server_t* server, uint8_t object, uint8_t team, vector3f_t pos)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket*  packet = enet_packet_create(NULL, 15, ENET_PACKET_FLAG_RELIABLE);
    datastream_t stream = {packet->data, packet->dataLength, 0};
    datastream_write_u8(&stream, PACKET_TYPE_MOVE_OBJECT);
    datastream_write_u8(&stream, object);
    datastream_write_u8(&stream, team);
    datastream_write_f(&stream, pos.x);
    datastream_write_f(&stream, pos.y);
    datastream_write_f(&stream, pos.z);

    uint8_t sent = 0;
    for (int player = 0; player < server->protocol.max_players; ++player) {
        if (is_past_state_data(server, player)) {
            if (enet_peer_send(server->player[player].peer, 0, packet) == 0) {
                sent = 1;
            }
        }
    }
    if (sent == 0) {
        enet_packet_destroy(packet);
    }
}

void send_intel_capture(server_t* server, uint8_t playerID, uint8_t winning)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    uint8_t team;
    if (server->player[playerID].team == 0) {
        team = 1;
    } else {
        team = 0;
    }
    if (server->player[playerID].has_intel == 0 || server->protocol.gamemode.intel_held[team] == 0) {
        return;
    }
    ENetPacket*  packet = enet_packet_create(NULL, 3, ENET_PACKET_FLAG_RELIABLE);
    datastream_t stream = {packet->data, packet->dataLength, 0};
    datastream_write_u8(&stream, PACKET_TYPE_INTEL_CAPTURE);
    datastream_write_u8(&stream, playerID);
    datastream_write_u8(&stream, winning);
    server->player[playerID].has_intel         = 0;
    server->protocol.gamemode.intel_held[team] = 0;

    uint8_t sent = 0;
    for (int player = 0; player < server->protocol.max_players; ++player) {
        if (is_past_state_data(server, player)) {
            if (enet_peer_send(server->player[player].peer, 0, packet) == 0) {
                sent = 1;
            }
        }
    }
    if (sent == 0) {
        enet_packet_destroy(packet);
    }
}

void send_intel_pickup(server_t* server, uint8_t playerID)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    uint8_t team;
    if (server->player[playerID].team == 0) {
        team = 1;
    } else {
        team = 0;
    }
    if (server->player[playerID].has_intel == 1 || server->protocol.gamemode.intel_held[team] == 1) {
        return;
    }
    ENetPacket*  packet = enet_packet_create(NULL, 2, ENET_PACKET_FLAG_RELIABLE);
    datastream_t stream = {packet->data, packet->dataLength, 0};
    datastream_write_u8(&stream, PACKET_TYPE_INTEL_PICKUP);
    datastream_write_u8(&stream, playerID);
    server->player[playerID].has_intel                                         = 1;
    server->protocol.gamemode.player_intel_team[server->player[playerID].team] = playerID;
    server->protocol.gamemode.intel_held[team]                                 = 1;

    uint8_t sent = 0;
    for (int player = 0; player < server->protocol.max_players; ++player) {
        if (is_past_state_data(server, player)) {
            if (enet_peer_send(server->player[player].peer, 0, packet) == 0) {
                sent = 1;
            }
        }
    }
    if (sent == 0) {
        enet_packet_destroy(packet);
    }
}

void send_intel_drop(server_t* server, uint8_t playerID)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    uint8_t team;
    if (server->player[playerID].team == 0) {
        team = 1;
    } else {
        team = 0;
    }
    if (server->player[playerID].has_intel == 0 || server->protocol.gamemode.intel_held[team] == 0) {
        return;
    }
    ENetPacket*  packet = enet_packet_create(NULL, 14, ENET_PACKET_FLAG_RELIABLE);
    datastream_t stream = {packet->data, packet->dataLength, 0};
    datastream_write_u8(&stream, PACKET_TYPE_INTEL_DROP);
    datastream_write_u8(&stream, playerID);
    if (server->protocol.current_gamemode == GAME_MODE_BABEL) {
        datastream_write_f(&stream, (float) server->s_map.map.size_x / 2);
        datastream_write_f(&stream, (float) server->s_map.map.size_y / 2);
        datastream_write_f(
        &stream,
        (float) mapvxl_find_top_block(&server->s_map.map, server->s_map.map.size_x / 2, server->s_map.map.size_y / 2));

        server->protocol.gamemode.intel[team].x = (float) server->s_map.map.size_x / 2;
        server->protocol.gamemode.intel[team].y = (float) server->s_map.map.size_y / 2;
        server->protocol.gamemode.intel[team].z =
        mapvxl_find_top_block(&server->s_map.map, server->s_map.map.size_x / 2, server->s_map.map.size_y / 2);
        server->protocol.gamemode.intel[server->player[playerID].team] = server->protocol.gamemode.intel[team];
        send_move_object(
        server, server->player[playerID].team, server->player[playerID].team, server->protocol.gamemode.intel[team]);
    } else {
        datastream_write_f(&stream, server->player[playerID].movement.position.x);
        datastream_write_f(&stream, server->player[playerID].movement.position.y);
        datastream_write_f(&stream,
                           (float) mapvxl_find_top_block(&server->s_map.map,
                                                         server->player[playerID].movement.position.x,
                                                         server->player[playerID].movement.position.y));

        server->protocol.gamemode.intel[team].x = (int) server->player[playerID].movement.position.x;
        server->protocol.gamemode.intel[team].y = (int) server->player[playerID].movement.position.y;
        server->protocol.gamemode.intel[team].z = mapvxl_find_top_block(
        &server->s_map.map, server->player[playerID].movement.position.x, server->player[playerID].movement.position.y);
    }
    server->player[playerID].has_intel                                         = 0;
    server->protocol.gamemode.player_intel_team[server->player[playerID].team] = 32;
    server->protocol.gamemode.intel_held[team]                                 = 0;

    LOG_INFO("Dropping intel at X: %d, Y: %d, Z: %d",
             (int) server->protocol.gamemode.intel[team].x,
             (int) server->protocol.gamemode.intel[team].y,
             (int) server->protocol.gamemode.intel[team].z);
    uint8_t sent = 0;
    for (int player = 0; player < server->protocol.max_players; ++player) {
        if (is_past_state_data(server, player)) {
            if (enet_peer_send(server->player[player].peer, 0, packet) == 0) {
                sent = 1;
            }
        }
    }
    if (sent == 0) {
        enet_packet_destroy(packet);
    }
}

void send_grenade(server_t* server, uint8_t playerID, float fuse, vector3f_t position, vector3f_t velocity)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket*  packet = enet_packet_create(NULL, 30, ENET_PACKET_FLAG_RELIABLE);
    datastream_t stream = {packet->data, packet->dataLength, 0};
    datastream_write_u8(&stream, PACKET_TYPE_GRENADE_PACKET);
    datastream_write_u8(&stream, playerID);
    datastream_write_f(&stream, fuse);
    datastream_write_f(&stream, position.x);
    datastream_write_f(&stream, position.y);
    datastream_write_f(&stream, position.z);
    datastream_write_f(&stream, velocity.x);
    datastream_write_f(&stream, velocity.y);
    datastream_write_f(&stream, velocity.z);
    if (SendPacketExceptSender(server, packet, playerID) == 0) {
        enet_packet_destroy(packet);
    }
}

void send_player_left(server_t* server, uint8_t playerID)
{
    char ipString[17];
    format_ip_to_str(ipString, server->player[playerID].ip);
    LOG_INFO("Player %s (%s, #%hhu) disconnected", server->player[playerID].name, ipString, playerID);
    if (server->protocol.num_players == 0) {
        return;
    }
    for (uint8_t i = 0; i < server->protocol.max_players; ++i) {
        if (i != playerID && is_past_state_data(server, i)) {
            ENetPacket*  packet = enet_packet_create(NULL, 2, ENET_PACKET_FLAG_RELIABLE);
            datastream_t stream = {packet->data, packet->dataLength, 0};
            datastream_write_u8(&stream, PACKET_TYPE_PLAYER_LEFT);
            datastream_write_u8(&stream, playerID);

            if (enet_peer_send(server->player[i].peer, 0, packet) != 0) {
                LOG_WARNING("Failed to send player left event");
                enet_packet_destroy(packet);
            }
        }
    }
}

void send_weapon_reload(server_t* server, uint8_t playerID, uint8_t startAnimation, uint8_t clip, uint8_t reserve)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket*  packet = enet_packet_create(NULL, 4, ENET_PACKET_FLAG_RELIABLE);
    datastream_t stream = {packet->data, packet->dataLength, 0};
    datastream_write_u8(&stream, PACKET_TYPE_WEAPON_RELOAD);
    datastream_write_u8(&stream, playerID);
    if (startAnimation) {
        datastream_write_u8(&stream, clip);
        datastream_write_u8(&stream, reserve);
    } else {
        datastream_write_u8(&stream, server->player[playerID].weapon_clip);
        datastream_write_u8(&stream, server->player[playerID].weapon_reserve);
    }
    if (startAnimation) {
        uint8_t sendSucc = 0;
        for (int player = 0; player < server->protocol.max_players; ++player) {
            if (is_past_state_data(server, player) && player != playerID) {
                if (enet_peer_send(server->player[player].peer, 0, packet) == 0) {
                    sendSucc = 1;
                }
            }
        }
        if (sendSucc == 0) {
            enet_packet_destroy(packet);
        }
    } else {
        if (is_past_join_screen(server, playerID)) {
            if (enet_peer_send(server->player[playerID].peer, 0, packet) != 0) {
                enet_packet_destroy(packet);
            }
        }
    }
}

void send_weapon_input(server_t* server, uint8_t playerID, uint8_t wInput)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket*  packet = enet_packet_create(NULL, 3, ENET_PACKET_FLAG_RELIABLE);
    datastream_t stream = {packet->data, packet->dataLength, 0};
    datastream_write_u8(&stream, PACKET_TYPE_WEAPON_INPUT);
    datastream_write_u8(&stream, playerID);
    if (server->player[playerID].sprinting) {
        wInput = 0;
    }
    datastream_write_u8(&stream, wInput);
    if (SendPacketExceptSender(server, packet, playerID) == 0) {
        enet_packet_destroy(packet);
    }
}

void send_set_color(server_t* server, uint8_t playerID, uint8_t R, uint8_t G, uint8_t B)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket*  packet = enet_packet_create(NULL, 5, ENET_PACKET_FLAG_RELIABLE);
    datastream_t stream = {packet->data, packet->dataLength, 0};
    datastream_write_u8(&stream, PACKET_TYPE_SET_COLOR);
    datastream_write_u8(&stream, playerID);
    datastream_write_u8(&stream, B);
    datastream_write_u8(&stream, G);
    datastream_write_u8(&stream, R);
    if (SendPacketExceptSender(server, packet, playerID) == 0) {
        enet_packet_destroy(packet);
    }
}

void send_set_color_to_player(server_t* server, uint8_t playerID, uint8_t sendToID, uint8_t R, uint8_t G, uint8_t B)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket*  packet = enet_packet_create(NULL, 5, ENET_PACKET_FLAG_RELIABLE);
    datastream_t stream = {packet->data, packet->dataLength, 0};
    datastream_write_u8(&stream, PACKET_TYPE_SET_COLOR);
    datastream_write_u8(&stream, playerID);
    datastream_write_u8(&stream, B);
    datastream_write_u8(&stream, G);
    datastream_write_u8(&stream, R);
    if (enet_peer_send(server->player[sendToID].peer, 0, packet) != 0) {
        enet_packet_destroy(packet);
    }
}

void send_set_tool(server_t* server, uint8_t playerID, uint8_t tool)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket*  packet = enet_packet_create(NULL, 3, ENET_PACKET_FLAG_RELIABLE);
    datastream_t stream = {packet->data, packet->dataLength, 0};
    datastream_write_u8(&stream, PACKET_TYPE_SET_TOOL);
    datastream_write_u8(&stream, playerID);
    datastream_write_u8(&stream, tool);
    if (SendPacketExceptSender(server, packet, playerID) == 0) {
        enet_packet_destroy(packet);
    }
}

void send_block_line(server_t* server, uint8_t playerID, vector3i_t start, vector3i_t end)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket*  packet = enet_packet_create(NULL, 26, ENET_PACKET_FLAG_RELIABLE);
    datastream_t stream = {packet->data, packet->dataLength, 0};
    datastream_write_u8(&stream, PACKET_TYPE_BLOCK_LINE);
    datastream_write_u8(&stream, playerID);
    datastream_write_u32(&stream, start.x);
    datastream_write_u32(&stream, start.y);
    datastream_write_u32(&stream, start.z);
    datastream_write_u32(&stream, end.x);
    datastream_write_u32(&stream, end.y);
    datastream_write_u32(&stream, end.z);
    uint8_t sent = 0;
    for (int player = 0; player < server->protocol.max_players; ++player) {
        if (is_past_state_data(server, player)) {
            if (enet_peer_send(server->player[player].peer, 0, packet) == 0) {
                sent = 1;
            }
        } else if (server->player[player].state == STATE_STARTING_MAP ||
                   server->player[player].state == STATE_LOADING_CHUNKS) {
            block_node_t* node   = (block_node_t*) malloc(sizeof(*node));
            node->position.x     = start.x;
            node->position.y     = start.y;
            node->position.z     = start.z;
            node->position_end.x = end.x;
            node->position_end.y = end.y;
            node->position_end.z = end.z;
            node->color          = server->player[playerID].color;
            node->type           = 10;
            node->sender_id      = playerID;
            LL_APPEND(server->player[player].blockBuffer, node);
        }
    }
    if (sent == 0) {
        enet_packet_destroy(packet);
    }
}

void block_line_to_player(server_t* server, uint8_t playerID, uint8_t sendToID, vector3i_t start, vector3i_t end)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket*  packet = enet_packet_create(NULL, 26, ENET_PACKET_FLAG_RELIABLE);
    datastream_t stream = {packet->data, packet->dataLength, 0};
    datastream_write_u8(&stream, PACKET_TYPE_BLOCK_LINE);
    datastream_write_u8(&stream, playerID);
    datastream_write_u32(&stream, start.x);
    datastream_write_u32(&stream, start.y);
    datastream_write_u32(&stream, start.z);
    datastream_write_u32(&stream, end.x);
    datastream_write_u32(&stream, end.y);
    datastream_write_u32(&stream, end.z);
    uint8_t sent = 0;
    if (enet_peer_send(server->player[sendToID].peer, 0, packet) == 0) {
        sent = 1;
    }
    if (sent == 0) {
        enet_packet_destroy(packet);
    }
}

void send_block_action(server_t* server, uint8_t playerID, uint8_t actionType, int X, int Y, int Z)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket*  packet = enet_packet_create(NULL, 15, ENET_PACKET_FLAG_RELIABLE);
    datastream_t stream = {packet->data, packet->dataLength, 0};
    datastream_write_u8(&stream, PACKET_TYPE_BLOCK_ACTION);
    datastream_write_u8(&stream, playerID);
    datastream_write_u8(&stream, actionType);
    datastream_write_u32(&stream, X);
    datastream_write_u32(&stream, Y);
    datastream_write_u32(&stream, Z);
    uint8_t sent = 0;
    for (int player = 0; player < server->protocol.max_players; ++player) {
        if (is_past_state_data(server, player)) {
            if (enet_peer_send(server->player[player].peer, 0, packet) == 0) {
                sent = 1;
            }
        } else if (server->player[player].state == STATE_STARTING_MAP ||
                   server->player[player].state == STATE_LOADING_CHUNKS) {
            block_node_t* node = (block_node_t*) malloc(sizeof(*node));
            node->position.x   = X;
            node->position.y   = Y;
            node->position.z   = Z;
            node->color        = server->player[playerID].color;
            node->type         = actionType;
            node->sender_id    = playerID;
            LL_APPEND(server->player[player].blockBuffer, node);
        }
    }
    if (sent == 0) {
        enet_packet_destroy(packet);
    }
}
void send_block_action_to_player(server_t* server,
                                 uint8_t   playerID,
                                 uint8_t   sendToID,
                                 uint8_t   actionType,
                                 int       X,
                                 int       Y,
                                 int       Z)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket*  packet = enet_packet_create(NULL, 15, ENET_PACKET_FLAG_RELIABLE);
    datastream_t stream = {packet->data, packet->dataLength, 0};
    datastream_write_u8(&stream, PACKET_TYPE_BLOCK_ACTION);
    datastream_write_u8(&stream, playerID);
    datastream_write_u8(&stream, actionType);
    datastream_write_u32(&stream, X);
    datastream_write_u32(&stream, Y);
    datastream_write_u32(&stream, Z);
    uint8_t sent = 0;
    if (enet_peer_send(server->player[sendToID].peer, 0, packet) == 0) {
        sent = 1;
    }
    if (sent == 0) {
        enet_packet_destroy(packet);
    }
}

void send_state_data(server_t* server, uint8_t playerID)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket*  packet = enet_packet_create(NULL, 104, ENET_PACKET_FLAG_RELIABLE);
    datastream_t stream = {packet->data, packet->dataLength, 0};
    datastream_write_u8(&stream, PACKET_TYPE_STATE_DATA);
    datastream_write_u8(&stream, playerID);
    datastream_write_color_rgb(&stream, server->protocol.color_fog);
    datastream_write_color_rgb(&stream, server->protocol.color_team[0]);
    datastream_write_color_rgb(&stream, server->protocol.color_team[1]);
    datastream_write_array(&stream, server->protocol.name_team[0], 10);
    datastream_write_array(&stream, server->protocol.name_team[1], 10);
    if (server->protocol.current_gamemode == 0 || server->protocol.current_gamemode == 1) {
        datastream_write_u8(&stream, server->protocol.current_gamemode);
    } else {
        datastream_write_u8(&stream, 0);
    }

    // MODE CTF:

    datastream_write_u8(&stream, server->protocol.gamemode.score[0]);    // SCORE TEAM A
    datastream_write_u8(&stream, server->protocol.gamemode.score[1]);    // SCORE TEAM B
    datastream_write_u8(&stream, server->protocol.gamemode.score_limit); // SCORE LIMIT

    server->protocol.gamemode.intel_flags = 0;

    if (server->protocol.gamemode.intel_held[0]) {
        server->protocol.gamemode.intel_flags = INTEL_TEAM_B;
    } else if (server->protocol.gamemode.intel_held[1]) {
        server->protocol.gamemode.intel_flags = INTEL_TEAM_A;
    } else if (server->protocol.gamemode.intel_held[0] && server->protocol.gamemode.intel_held[1]) {
        server->protocol.gamemode.intel_flags = INTEL_TEAM_BOTH;
    }

    datastream_write_u8(&stream, server->protocol.gamemode.intel_flags); // INTEL FLAGS

    if ((server->protocol.gamemode.intel_flags & 1) == 0) {
        datastream_write_u8(&stream, server->protocol.gamemode.player_intel_team[1]);
        for (int i = 0; i < 11; ++i) {
            datastream_write_u8(&stream, 255);
        }
    } else {
        datastream_write_vector3f(&stream, server->protocol.gamemode.intel[0]);
    }

    if ((server->protocol.gamemode.intel_flags & 2) == 0) {
        datastream_write_u8(&stream, server->protocol.gamemode.player_intel_team[0]);
        for (int i = 0; i < 11; ++i) {
            datastream_write_u8(&stream, 255);
        }
    } else {
        datastream_write_vector3f(&stream, server->protocol.gamemode.intel[1]);
    }

    datastream_write_vector3f(&stream, server->protocol.gamemode.base[0]);
    datastream_write_vector3f(&stream, server->protocol.gamemode.base[1]);

    if (enet_peer_send(server->player[playerID].peer, 0, packet) == 0) {
        server->player[playerID].state = STATE_PICK_SCREEN;
    } else {
        enet_packet_destroy(packet);
    }
}

void send_input_data(server_t* server, uint8_t playerID)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket*  packet = enet_packet_create(NULL, 3, ENET_PACKET_FLAG_RELIABLE);
    datastream_t stream = {packet->data, packet->dataLength, 0};
    datastream_write_u8(&stream, PACKET_TYPE_INPUT_DATA);
    datastream_write_u8(&stream, playerID);
    datastream_write_u8(&stream, server->player[playerID].input);
    if (SendPacketExceptSenderDistCheck(server, packet, playerID) == 0) {
        enet_packet_destroy(packet);
    }
}

void send_kill_action_packet(server_t* server,
                             uint8_t   killerID,
                             uint8_t   playerID,
                             uint8_t   killReason,
                             uint8_t   respawnTime,
                             uint8_t   makeInvisible)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    if (server->player[playerID].alive == 0) {
        return; // Cant kill player if they are dead
    }
    ENetPacket*  packet = enet_packet_create(NULL, 5, ENET_PACKET_FLAG_RELIABLE);
    datastream_t stream = {packet->data, packet->dataLength, 0};
    datastream_write_u8(&stream, PACKET_TYPE_KILL_ACTION);
    datastream_write_u8(&stream, playerID);    // Player that died.
    datastream_write_u8(&stream, killerID);    // Player that killed.
    datastream_write_u8(&stream, killReason);  // Killing reason (1 is headshot)
    datastream_write_u8(&stream, respawnTime); // Time before respawn happens
    uint8_t sent = 0;
    for (int player = 0; player < server->protocol.max_players; ++player) {
        uint8_t isPast = is_past_state_data(server, player);
        if ((makeInvisible && player != playerID && isPast) || (isPast && !makeInvisible)) {
            if (enet_peer_send(server->player[player].peer, 0, packet) == 0) {
                sent = 1;
            }
        }
    }
    if (sent == 0) {
        enet_packet_destroy(packet);
        return; // Do not kill the player since sending the packet failed
    }
    if (!makeInvisible && server->player[playerID].is_invisible == 0) {
        if (killerID != playerID) {
            server->player[killerID].kills++;
        }
        server->player[playerID].deaths++;
        server->player[playerID].alive                        = 0;
        server->player[playerID].respawn_time                 = respawnTime;
        server->player[playerID].timers.start_of_respawn_wait = time(NULL);
        server->player[playerID].state                        = STATE_WAITING_FOR_RESPAWN;
        switch (server->player[playerID].weapon) {
            case 0:
                server->player[playerID].weapon_reserve  = 50;
                server->player[playerID].weapon_clip     = 10;
                server->player[playerID].default_clip    = RIFLE_DEFAULT_CLIP;
                server->player[playerID].default_reserve = RIFLE_DEFAULT_RESERVE;
                break;
            case 1:
                server->player[playerID].weapon_reserve  = 120;
                server->player[playerID].weapon_clip     = 30;
                server->player[playerID].default_clip    = SMG_DEFAULT_CLIP;
                server->player[playerID].default_reserve = SMG_DEFAULT_RESERVE;
                break;
            case 2:
                server->player[playerID].weapon_reserve  = 48;
                server->player[playerID].weapon_clip     = 6;
                server->player[playerID].default_clip    = SHOTGUN_DEFAULT_CLIP;
                server->player[playerID].default_reserve = SHOTGUN_DEFAULT_RESERVE;
                break;
        }
    }
    if (server->player[playerID].has_intel) {
        send_intel_drop(server, playerID);
    }
}

void send_set_hp(server_t*  server,
                 uint8_t    playerID,
                 uint8_t    hitPlayerID,
                 long       HPChange,
                 uint8_t    typeOfDamage,
                 uint8_t    killReason,
                 uint8_t    respawnTime,
                 uint8_t    isGrenade,
                 vector3f_t position)
{
    if (server->protocol.num_players == 0 || server->player[hitPlayerID].team == TEAM_SPECTATOR ||
        (!server->player[playerID].allow_team_killing &&
         server->player[playerID].team == server->player[hitPlayerID].team && playerID != hitPlayerID))
    {
        return;
    }
    if ((server->player[playerID].allow_killing && server->global_ak && server->player[playerID].allow_killing &&
         server->player[playerID].alive) ||
        typeOfDamage == 0)
    {
        if (HPChange > server->player[hitPlayerID].hp) {
            HPChange = server->player[hitPlayerID].hp;
        }
        server->player[hitPlayerID].hp -= HPChange;
        if (server->player[hitPlayerID].hp < 0) // We should NEVER return true here. If we do stuff is really broken
            server->player[hitPlayerID].hp = 0;

        else if (server->player[hitPlayerID].hp > 100) // Same as above
            server->player[hitPlayerID].hp = 100;

        if (server->player[hitPlayerID].hp == 0) {
            send_kill_action_packet(server, playerID, hitPlayerID, killReason, respawnTime, 0);
            return;
        }

        else if (server->player[hitPlayerID].hp > 0 && server->player[hitPlayerID].hp < 100)
        {
            ENetPacket*  packet = enet_packet_create(NULL, 15, ENET_PACKET_FLAG_RELIABLE);
            datastream_t stream = {packet->data, packet->dataLength, 0};
            datastream_write_u8(&stream, PACKET_TYPE_SET_HP);
            datastream_write_u8(&stream, server->player[hitPlayerID].hp);
            datastream_write_u8(&stream, typeOfDamage);
            if (typeOfDamage != 0 && isGrenade == 0) {
                datastream_write_f(&stream, server->player[playerID].movement.position.x);
                datastream_write_f(&stream, server->player[playerID].movement.position.y);
                datastream_write_f(&stream, server->player[playerID].movement.position.z);
            } else if (typeOfDamage != 0 && isGrenade == 1) {
                datastream_write_f(&stream, position.x);
                datastream_write_f(&stream, position.y);
                datastream_write_f(&stream, position.z);
            } else {
                datastream_write_f(&stream, 0);
                datastream_write_f(&stream, 0);
                datastream_write_f(&stream, 0);
            }
            if (enet_peer_send(server->player[hitPlayerID].peer, 0, packet) != 0) {
                enet_packet_destroy(packet);
            }
        }
    }
}

void send_existing_player(server_t* server, uint8_t playerID, uint8_t otherID)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket*  packet = enet_packet_create(NULL, 28, ENET_PACKET_FLAG_RELIABLE);
    datastream_t stream = {packet->data, packet->dataLength, 0};
    datastream_write_u8(&stream, PACKET_TYPE_EXISTING_PLAYER);
    datastream_write_u8(&stream, otherID);                              // ID
    datastream_write_u8(&stream, server->player[otherID].team);         // TEAM
    datastream_write_u8(&stream, server->player[otherID].weapon);       // WEAPON
    datastream_write_u8(&stream, server->player[otherID].item);         // HELD ITEM
    datastream_write_u32(&stream, server->player[otherID].kills);       // KILLS
    datastream_write_color_rgb(&stream, server->player[otherID].color); // COLOR
    datastream_write_array(&stream, server->player[otherID].name, 16);  // NAME

    if (enet_peer_send(server->player[playerID].peer, 0, packet) != 0) {
        LOG_WARNING("Failed to send player state");
        enet_packet_destroy(packet);
    }
}

void sendVersionRequest(server_t* server, uint8_t playerID)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket*  packet = enet_packet_create(NULL, 1, ENET_PACKET_FLAG_RELIABLE);
    datastream_t stream = {packet->data, packet->dataLength, 0};
    datastream_write_u8(&stream, PACKET_TYPE_VERSION_REQUEST);
    if (enet_peer_send(server->player[playerID].peer, 0, packet) != 0) {
        enet_packet_destroy(packet);
    }
}
void send_map_start(server_t* server, uint8_t playerID)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    LOG_INFO("Sending map info to %s (#%hhu)", server->player[playerID].name, playerID);
    ENetPacket*  packet = enet_packet_create(NULL, 5, ENET_PACKET_FLAG_RELIABLE);
    datastream_t stream = {packet->data, packet->dataLength, 0};
    datastream_write_u8(&stream, PACKET_TYPE_MAP_START);
    datastream_write_u32(&stream, server->s_map.compressed_size);
    if (enet_peer_send(server->player[playerID].peer, 0, packet) == 0) {
        server->player[playerID].state = STATE_LOADING_CHUNKS;

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
        map                             = old_map;
        server->s_map.compressed_map    = compress_queue(map, server->s_map.map_size, DEFAULT_COMPRESS_CHUNK_SIZE);
        server->player[playerID].queues = server->s_map.compressed_map;
        free(map);
    }
}

void send_map_chunks(server_t* server, uint8_t playerID)
{
    if (server->player[playerID].queues == NULL) {
        while (server->s_map.compressed_map) {
            server->s_map.compressed_map = queue_pop(server->s_map.compressed_map);
        }
        sendVersionRequest(server, playerID);
        server->player[playerID].state = STATE_JOINING;
        LOG_INFO("Finished sending map to %s (#%hhu)", server->player[playerID].name, playerID);
    } else {
        ENetPacket* packet =
        enet_packet_create(NULL, server->player[playerID].queues->length + 1, ENET_PACKET_FLAG_RELIABLE);
        datastream_t stream = {packet->data, packet->dataLength, 0};
        datastream_write_u8(&stream, PACKET_TYPE_MAP_CHUNK);
        datastream_write_array(
        &stream, server->player[playerID].queues->block, server->player[playerID].queues->length);

        if (enet_peer_send(server->player[playerID].peer, 0, packet) == 0) {
            server->player[playerID].queues = server->player[playerID].queues->next;
        }
    }
}

void send_create_player(server_t* server, uint8_t playerID, uint8_t otherID)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket*  packet = enet_packet_create(NULL, 32, ENET_PACKET_FLAG_RELIABLE);
    datastream_t stream = {packet->data, packet->dataLength, 0};
    datastream_write_u8(&stream, PACKET_TYPE_CREATE_PLAYER);
    datastream_write_u8(&stream, otherID);                                         // ID
    datastream_write_u8(&stream, server->player[otherID].weapon);                  // WEAPON
    datastream_write_u8(&stream, server->player[otherID].team);                    // TEAM
    datastream_write_vector3f(&stream, server->player[otherID].movement.position); // X Y Z
    datastream_write_array(&stream, server->player[otherID].name, 16);             // NAME

    if (enet_peer_send(server->player[playerID].peer, 0, packet) != 0) {
        LOG_WARNING("Failed to send player state");
        enet_packet_destroy(packet);
    }
}

void send_respawn(server_t* server, uint8_t playerID)
{
    for (uint8_t i = 0; i < server->protocol.max_players; ++i) {
        if (is_past_join_screen(server, i)) {
            send_create_player(server, i, playerID);
        }
    }
    server->player[playerID].state = STATE_READY;
}

void receive_handle_send_message(server_t* server, uint8_t player, datastream_t* data)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    uint32_t packetSize = data->length;
    LOG_STATUS("%d %d", packetSize, data->length);
    uint8_t  ID       = datastream_read_u8(data);
    int      meantfor = datastream_read_u8(data);
    uint32_t length   = datastream_left(data);
    if (length > 2048) {
        length = 2048; // Lets limit messages to 2048 characters
    }
    char* message =
    calloc(length + 1, sizeof(char)); // Allocated 1 more byte in the case that client sent us non null ending string
    datastream_read_array(data, message, length);
    if (message[length - 1] != '\0') {
        message[length] = '\0';
        length++;
        packetSize = length + 3;
    }
    if (player != ID) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in message packet", player, ID);
    }
    uint8_t resetTime = 1;
    if (!diffIsOlderThenDontUpdate(
        get_nanos(), server->player[player].timers.since_last_message_from_spam, (uint64_t) NANO_IN_MILLI * 2000) &&
        !server->player[player].muted && server->player[player].permissions <= 1)
    {
        server->player[player].spam_counter++;
        if (server->player[player].spam_counter >= 5) {
            sendMessageToStaff(
            server, "WARNING: Player %s (#%d) is trying to spam. Muting.", server->player[player].name, player);
            send_server_notice(server,
                               player,
                               0,
                               "SERVER: You have been muted for excessive spam. If you feel like this is a mistake "
                               "contact staff via /admin command");
            server->player[player].muted        = 1;
            server->player[player].spam_counter = 0;
        }
        resetTime = 0;
    }
    if (resetTime) {
        server->player[player].timers.since_last_message_from_spam = get_nanos();
        server->player[player].spam_counter                        = 0;
    }

    if (!diffIsOlderThen(
        get_nanos(), &server->player[player].timers.since_last_message, (uint64_t) NANO_IN_MILLI * 400) &&
        server->player[player].permissions <= 1)
    {
        send_server_notice(
        server, player, 0, "WARNING: You sent last message too fast and thus was not sent out to players");
        return;
    }

    char meantFor[7];
    switch (meantfor) {
        case 0:
            snprintf(meantFor, 7, "Global");
            break;
        case 1:
            snprintf(meantFor, 5, "Team");
            break;
        case 2:
            snprintf(meantFor, 7, "System");
            break;
    }
    LOG_INFO("Player %s (#%hhu) (%s) said: %s", server->player[player].name, player, meantFor, message);

    uint8_t sent = 0;
    if (message[0] == '/') {
        command_handle(server, player, message, 0);
    } else {
        if (!server->player[player].muted) {
            ENetPacket*  packet = enet_packet_create(NULL, packetSize, ENET_PACKET_FLAG_RELIABLE);
            datastream_t stream = {packet->data, packet->dataLength, 0};
            datastream_write_u8(&stream, PACKET_TYPE_CHAT_MESSAGE);
            datastream_write_u8(&stream, player);
            datastream_write_u8(&stream, meantfor);
            datastream_write_array(&stream, message, length);
            for (int playerID = 0; playerID < server->protocol.max_players; ++playerID) {
                if (is_past_join_screen(server, playerID) && !server->player[player].muted &&
                    ((server->player[playerID].team == server->player[player].team && meantfor == 1) || meantfor == 0))
                {
                    if (enet_peer_send(server->player[playerID].peer, 0, packet) == 0) {
                        sent = 1;
                    }
                }
            }
            if (sent == 0) {
                enet_packet_destroy(packet);
            }
        }
    }
    free(message);
}

void send_world_update(server_t* server, uint8_t playerID)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket*  packet = enet_packet_create(NULL, 1 + (32 * 24), 0);
    datastream_t stream = {packet->data, packet->dataLength, 0};
    datastream_write_u8(&stream, PACKET_TYPE_WORLD_UPDATE);

    for (uint8_t j = 0; j < server->protocol.max_players; ++j) {
        if (playerToPlayerVisible(server, playerID, j) && server->player[j].is_invisible == 0) {
            /*float    dt       = (getNanos() - server->globalTimers.lastUpdateTime) / 1000000000.f;
            Vector3f position = {server->player[j].movement.velocity.x * dt + server->player[j].movement.position.x,
                                 server->player[j].movement.velocity.y * dt + server->player[j].movement.position.y,
                                 server->player[j].movement.velocity.z * dt + server->player[j].movement.position.z};
            WriteVector3f(&stream, position);*/
            datastream_write_vector3f(&stream, server->player[j].movement.position);
            datastream_write_vector3f(&stream, server->player[j].movement.forward_orientation);
        } else {
            vector3f_t empty;
            empty.x = 0;
            empty.y = 0;
            empty.z = 0;
            datastream_write_vector3f(&stream, empty);
            datastream_write_vector3f(&stream, empty);
        }
    }
    if (enet_peer_send(server->player[playerID].peer, 0, packet) != 0) {
        enet_packet_destroy(packet);
    }
}

void send_position_packet(server_t* server, uint8_t playerID, float x, float y, float z)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket*  packet = enet_packet_create(NULL, 13, ENET_PACKET_FLAG_RELIABLE);
    datastream_t stream = {packet->data, packet->dataLength, 0};
    datastream_write_u8(&stream, PACKET_TYPE_POSITION_DATA);
    datastream_write_f(&stream, x);
    datastream_write_f(&stream, y);
    datastream_write_f(&stream, z);
    if (enet_peer_send(server->player[playerID].peer, 0, packet) != 0) {
        enet_packet_destroy(packet);
    }
}

static void receive_grenade_packet(server_t* server, uint8_t playerID, datastream_t* data)
{
    uint8_t ID = datastream_read_u8(data);
    if (playerID != ID) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in grenade packet", playerID, ID);
    }
    if (server->player[playerID].primary_fire && server->player[playerID].item == 1) {
        send_server_notice(server, playerID, 0, "InstaSuicideNade detected. Grenade ineffective");
        sendMessageToStaff(
        server, 0, "Player %s (#%hhu) tried to use InstaSpadeNade", server->player[playerID].name, playerID);
        return;
    }
    uint64_t timeNow = get_nanos();
    if (!diffIsOlderThen(timeNow, &server->player[playerID].timers.since_last_grenade_thrown, NANO_IN_MILLI * 500) ||
        !diffIsOlderThenDontUpdate(
        timeNow, server->player[playerID].timers.since_possible_spade_nade, (long) NANO_IN_MILLI * 1000))
    {
        return;
    }
    grenade_t* grenade = malloc(sizeof(grenade_t));
    if (server->player[playerID].grenades > 0) {
        grenade->fuse       = datastream_read_f(data);
        grenade->position.x = datastream_read_f(data);
        grenade->position.y = datastream_read_f(data);
        grenade->position.z = datastream_read_f(data);
        float velX = datastream_read_f(data), velY = datastream_read_f(data), velZ = datastream_read_f(data);
        if (server->player[playerID].sprinting) {
            free(grenade);
            return;
        }
        grenade->velocity.x = velX;
        grenade->velocity.y = velY;
        grenade->velocity.z = velZ;
        vector3f_t velocity = {grenade->velocity.x - server->player[playerID].movement.velocity.x,
                               grenade->velocity.y - server->player[playerID].movement.velocity.y,
                               grenade->velocity.z - server->player[playerID].movement.velocity.z};
        float      length   = sqrt((velocity.x * velocity.x) + (velocity.y * velocity.y) + (velocity.z * velocity.z));
        if (length > 2) {
            float lenghtNorm = 1 / length;
            velocity.x       = ((velocity.x * lenghtNorm) * 2) + server->player[playerID].movement.velocity.x;
            velocity.y       = ((velocity.y * lenghtNorm) * 2) + server->player[playerID].movement.velocity.y;
            velocity.z       = ((velocity.z * lenghtNorm) * 2) + server->player[playerID].movement.velocity.z;
        }
        vector3f_t posZ1 = grenade->position;
        posZ1.z += 1;
        vector3f_t posZ2 = grenade->position;
        posZ2.z += 2;
        if (valid_pos_v3f(server, grenade->position) ||
            (valid_pos_v3f(server, posZ1) && server->player[playerID].movement.position.z < 0) ||
            (valid_pos_v3f(server, posZ2) && server->player[playerID].movement.position.z < 0))
        {
            send_grenade(server, playerID, grenade->fuse, grenade->position, grenade->velocity);
            grenade->sent            = 1;
            grenade->time_since_sent = get_nanos();
        }
        DL_APPEND(server->player[playerID].grenade, grenade);
        server->player[playerID].grenades--;
    }
}

// hitPlayerID is the player that got shot
// playerID is the player who fired.
static void receive_hit_packet(server_t* server, uint8_t playerID, datastream_t* data)
{
    uint8_t    hitPlayerID = datastream_read_u8(data);
    hit_t      hitType     = datastream_read_u8(data);
    vector3f_t shotPos     = server->player[playerID].movement.position;
    vector3f_t shotEyePos  = server->player[playerID].movement.eye_pos;
    vector3f_t hitPos      = server->player[hitPlayerID].movement.position;
    vector3f_t shotOrien   = server->player[playerID].movement.forward_orientation;
    float      distance    = DistanceIn2D(shotPos, hitPos);
    long       x = 0, y = 0, z = 0;

    if (server->player[playerID].sprinting ||
        (server->player[playerID].item == 2 && server->player[playerID].weapon_clip <= 0))
    {
        return; // Sprinting and hitting somebody is impossible
    }

    uint64_t timeNow = get_nanos();

    if (allowShot(server, playerID, hitPlayerID, timeNow, distance, &x, &y, &z, shotPos, shotOrien, hitPos, shotEyePos))
    {
        switch (server->player[playerID].weapon) {
            case WEAPON_RIFLE:
            {
                switch (hitType) {
                    case HIT_TYPE_HEAD:
                    {
                        send_kill_action_packet(server, playerID, hitPlayerID, 1, 5, 0);
                        break;
                    }
                    case HIT_TYPE_TORSO:
                    {
                        send_set_hp(server, playerID, hitPlayerID, 49, 1, 0, 5, 0, shotPos);
                        break;
                    }
                    case HIT_TYPE_ARMS:
                    {
                        send_set_hp(server, playerID, hitPlayerID, 33, 1, 0, 5, 0, shotPos);
                        break;
                    }
                    case HIT_TYPE_LEGS:
                    {
                        send_set_hp(server, playerID, hitPlayerID, 33, 1, 0, 5, 0, shotPos);
                        break;
                    }
                    case HIT_TYPE_MELEE:
                        // Empty so we dont have errors :)
                        break;
                }
                break;
            }
            case WEAPON_SMG:
            {
                switch (hitType) {
                    case HIT_TYPE_HEAD:
                    {
                        send_set_hp(server, playerID, hitPlayerID, 75, 1, 1, 5, 0, shotPos);
                        break;
                    }
                    case HIT_TYPE_TORSO:
                    {
                        send_set_hp(server, playerID, hitPlayerID, 29, 1, 0, 5, 0, shotPos);
                        break;
                    }
                    case HIT_TYPE_ARMS:
                    {
                        send_set_hp(server, playerID, hitPlayerID, 18, 1, 0, 5, 0, shotPos);
                        break;
                    }
                    case HIT_TYPE_LEGS:
                    {
                        send_set_hp(server, playerID, hitPlayerID, 18, 1, 0, 5, 0, shotPos);
                        break;
                    }
                    case HIT_TYPE_MELEE:
                        // Empty so we dont have errors :)
                        break;
                }
                break;
            }
            case WEAPON_SHOTGUN:
            {
                switch (hitType) {
                    case HIT_TYPE_HEAD:
                    {
                        send_set_hp(server, playerID, hitPlayerID, 37, 1, 1, 5, 0, shotPos);
                        break;
                    }
                    case HIT_TYPE_TORSO:
                    {
                        send_set_hp(server, playerID, hitPlayerID, 27, 1, 0, 5, 0, shotPos);
                        break;
                    }
                    case HIT_TYPE_ARMS:
                    {
                        send_set_hp(server, playerID, hitPlayerID, 16, 1, 0, 5, 0, shotPos);
                        break;
                    }
                    case HIT_TYPE_LEGS:
                    {
                        send_set_hp(server, playerID, hitPlayerID, 16, 1, 0, 5, 0, shotPos);
                        break;
                    }
                    case HIT_TYPE_MELEE:
                        // Empty so we dont have errors :)
                        break;
                }
                break;
            }
        }
        if (hitType == HIT_TYPE_MELEE) {
            send_set_hp(server, playerID, hitPlayerID, 80, 1, 2, 5, 0, shotPos);
        }
    }
}

static void receive_orientation_data(server_t* server, uint8_t playerID, datastream_t* data)
{
    float x, y, z;
    x                = datastream_read_f(data);
    y                = datastream_read_f(data);
    z                = datastream_read_f(data);
    float length     = sqrt((x * x) + (y * y) + (z * z));
    float normLength = 1 / length;
    // Normalize the vectors if their length > 1
    if (length > 1.f) {
        server->player[playerID].movement.forward_orientation.x = x * normLength;
        server->player[playerID].movement.forward_orientation.y = y * normLength;
        server->player[playerID].movement.forward_orientation.z = z * normLength;
    } else {
        server->player[playerID].movement.forward_orientation.x = x;
        server->player[playerID].movement.forward_orientation.y = y;
        server->player[playerID].movement.forward_orientation.z = z;
    }

    physics_reorient_player(server, playerID, &server->player[playerID].movement.forward_orientation);
}

static void receive_input_data(server_t* server, uint8_t playerID, datastream_t* data)
{
    uint8_t bits[8];
    uint8_t mask = 1;
    uint8_t ID;
    ID = datastream_read_u8(data);
    if (playerID != ID) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in Input packet", playerID, ID);
    } else if (server->player[playerID].state == STATE_READY) {
        server->player[playerID].input = datastream_read_u8(data);
        for (int i = 0; i < 8; i++) {
            bits[i] = (server->player[playerID].input >> i) & mask;
        }
        server->player[playerID].move_forward   = bits[0];
        server->player[playerID].move_backwards = bits[1];
        server->player[playerID].move_left      = bits[2];
        server->player[playerID].move_right     = bits[3];
        server->player[playerID].jumping        = bits[4];
        server->player[playerID].crouching      = bits[5];
        server->player[playerID].sneaking       = bits[6];
        server->player[playerID].sprinting      = bits[7];
        send_input_data(server, playerID);
    }
}

static void receive_position_data(server_t* server, uint8_t playerID, datastream_t* data)
{
    float x, y, z;
    x = datastream_read_f(data);
    y = datastream_read_f(data);
    z = datastream_read_f(data);
#ifdef DEBUG
    LOG_DEBUG("Player: %d, Our X: %f, Y: %f, Z: %f Actual X: %f, Y: %f, Z: %f",
              playerID,
              server->player[playerID].movement.position.x,
              server->player[playerID].movement.position.y,
              server->player[playerID].movement.position.z,
              x,
              y,
              z);
    LOG_DEBUG("Player: %d, Z solid: %d, Z+1 solid: %d, Z+2 solid: %d, Z: %d, Z+1: %d, Z+2: %d, Crouching: %d",
              playerID,
              mapvxlIsSolid(&server->s_map.map, (int) x, (int) y, (int) z),
              mapvxlIsSolid(&server->s_map.map, (int) x, (int) y, (int) z + 1),
              mapvxlIsSolid(&server->s_map.map, (int) x, (int) y, (int) z + 2),
              (int) z,
              (int) z + 1,
              (int) z + 2,
              server->player[playerID].crouching);
#endif
    server->player[playerID].movement.prev_position = server->player[playerID].movement.position;
    server->player[playerID].movement.position.x    = x;
    server->player[playerID].movement.position.y    = y;
    server->player[playerID].movement.position.z    = z;
    /*uint8_t resetTime                                = 1;
    if (!diffIsOlderThenDontUpdate(
        getNanos(), server->player[playerID].timers.duringNoclipPeriod, (uint64_t) NANO_IN_SECOND * 10))
    {
        resetTime = 0;
        if (validPlayerPos(server, playerID, x, y, z) == 0) {
            server->player[playerID].invalidPosCount++;
            if (server->player[playerID].invalidPosCount == 5) {
                sendMessageToStaff(server, "Player %s (#%hhu) is most likely trying to avoid noclip detection");
            }
        }
    }
    if (resetTime) {
        server->player[playerID].timers.duringNoclipPeriod = getNanos();
        server->player[playerID].invalidPosCount           = 0;
    }*/

    if (valid_pos_3f(server, playerID, x, y, z)) {
        server->player[playerID].movement.prev_legit_pos = server->player[playerID].movement.position;
    } /*else if (validPlayerPos(server,
                              playerID,
                              server->player[playerID].movement.position.x,
                              server->player[playerID].movement.position.y,
                              server->player[playerID].movement.position.z) == 0 &&
               validPlayerPos(server,
                              playerID,
                              server->player[playerID].movement.prevPosition.x,
                              server->player[playerID].movement.prevPosition.y,
                              server->player[playerID].movement.prevPosition.z) == 0)
    {
        LOG_WARNING("Player %s (#%hhu) may be noclipping!", server->player[playerID].name, playerID);
        sendMessageToStaff(server, "Player %s (#%d) may be noclipping!", server->player[playerID].name, playerID);
        if (validPlayerPos(server,
                           playerID,
                           server->player[playerID].movement.prevLegitPos.x,
                           server->player[playerID].movement.prevLegitPos.y,
                           server->player[playerID].movement.prevLegitPos.z) == 0)
        {
            if (getPlayerUnstuck(server, playerID) == 0) {
                SetPlayerRespawnPoint(server, playerID);
                sendServerNotice(
                server, playerID, 0, "Server could not find a free position to get you unstuck. Reseting to spawn");
                LOG_WARNING(
                "Could not find legit position for player %s (#%d) to get them unstuck. Resetting to spawn. "
                "Go check them!",
                server->player[playerID].name,
                playerID);
                sendMessageToStaff(
                server,
                "Could not find legit position for player %s (#%d) to get them unstuck. Resetting to spawn. "
                "Go check them!",
                server->player[playerID].name,
                playerID);
                server->player[playerID].movement.prevLegitPos = server->player[playerID].movement.position;
            }
        }
        SendPositionPacket(server,
                           playerID,
                           server->player[playerID].movement.prevLegitPos.x,
                           server->player[playerID].movement.prevLegitPos.y,
                           server->player[playerID].movement.prevLegitPos.z);
    }*/
}

static void receive_existing_player(server_t* server, uint8_t playerID, datastream_t* data)
{
    datastream_skip(data, 1); // Clients always send a "dumb" ID here since server has not sent them their ID yet

    server->player[playerID].team   = datastream_read_u8(data);
    server->player[playerID].weapon = datastream_read_u8(data);
    server->player[playerID].item   = datastream_read_u8(data);
    server->player[playerID].kills  = datastream_read_u32(data);

    if (server->player[playerID].team != 0 && server->player[playerID].team != 1 &&
        server->player[playerID].team != 255) {
        LOG_WARNING(
        "Player %s (#%hhu) sent invalid team. Switching them to Spectator", server->player[playerID].name, playerID);
        server->player[playerID].team = 255;
    }

    server->protocol.num_team_users[server->player[playerID].team]++;

    datastream_read_color_rgb(data);
    server->player[playerID].ups = 60;

    uint32_t length  = datastream_left(data);
    uint8_t  invName = 0;
    if (length > 16) {
        LOG_WARNING("Name of player %d is too long. Cutting", playerID);
        length = 16;
    } else {
        server->player[playerID].name[length] = '\0';
        datastream_read_array(data, server->player[playerID].name, length);

        if (strlen(server->player[playerID].name) == 0) {
            snprintf(server->player[playerID].name, strlen("Deuce") + 1, "Deuce");
            length  = 5;
            invName = 1;
        } else if (server->player[playerID].name[0] == '#') {
            snprintf(server->player[playerID].name, strlen("Deuce") + 1, "Deuce");
            length  = 5;
            invName = 1;
        }

        char* lowerCaseName = malloc((strlen(server->player[playerID].name) + 1) * sizeof(char));
        snprintf(lowerCaseName, strlen(server->player[playerID].name), "%s", server->player[playerID].name);

        for (uint32_t i = 0; i < strlen(server->player[playerID].name); ++i)
            lowerCaseName[i] = tolower(lowerCaseName[i]);

        char* unwantedNames[] = {"igger", "1gger", "igg3r", "1gg3r", NULL};

        int index = 0;

        while (unwantedNames[index] != NULL) {
            if (strstr(lowerCaseName, unwantedNames[index]) != NULL &&
                strcmp(unwantedNames[index], strstr(lowerCaseName, unwantedNames[index])) == 0)
            {
                snprintf(server->player[playerID].name, strlen("Deuce") + 1, "Deuce");
                length  = 5;
                invName = 1;
                free(lowerCaseName);
                return;
            }
            index++;
        }

        free(lowerCaseName);
        int count = 0;
        for (uint8_t i = 0; i < server->protocol.max_players; i++) {
            if (is_past_join_screen(server, i) && i != playerID) {
                if (strcmp(server->player[playerID].name, server->player[i].name) == 0) {
                    count++;
                }
            }
        }
        if (count > 0) {
            char idChar[4];
            snprintf(idChar, 4, "%d", playerID);
            strlcat(server->player[playerID].name, idChar, 17);
        }
    }
    switch (server->player[playerID].weapon) {
        case 0:
            server->player[playerID].weapon_reserve  = 50;
            server->player[playerID].weapon_clip     = 10;
            server->player[playerID].default_clip    = RIFLE_DEFAULT_CLIP;
            server->player[playerID].default_reserve = RIFLE_DEFAULT_RESERVE;
            break;
        case 1:
            server->player[playerID].weapon_reserve  = 120;
            server->player[playerID].weapon_clip     = 30;
            server->player[playerID].default_clip    = SMG_DEFAULT_CLIP;
            server->player[playerID].default_reserve = SMG_DEFAULT_RESERVE;
            break;
        case 2:
            server->player[playerID].weapon_reserve  = 48;
            server->player[playerID].weapon_clip     = 6;
            server->player[playerID].default_clip    = SHOTGUN_DEFAULT_CLIP;
            server->player[playerID].default_reserve = SHOTGUN_DEFAULT_RESERVE;
            break;
    }
    server->player[playerID].state = STATE_SPAWNING;
    char IP[17];
    format_ip_to_str(IP, server->player[playerID].ip);
    char team[15];
    team_id_to_str(server, team, server->player[playerID].team);
    LOG_INFO("Player %s (%s, #%hhu) joined %s", server->player[playerID].name, IP, playerID, team);
    if (server->player[playerID].welcome_sent == 0) {
        string_node_t* welcomeMessage;
        DL_FOREACH(server->welcome_messages, welcomeMessage)
        {
            send_server_notice(server, playerID, 0, welcomeMessage->string);
        }
        if (invName) {
            send_server_notice(
            server,
            playerID,
            0,
            "Your name was either empty, had # in front of it or contained something nasty. Your name "
            "has been set to %s",
            server->player[playerID].name);
        }
        server->player[playerID].welcome_sent = 1; // So we dont send the message to the player on each time they spawn.
    }

    if (server->protocol.gamemode.intel_held[0] == 0) {
        send_move_object(server, 0, 0, server->protocol.gamemode.intel[0]);
    }
    if (server->protocol.gamemode.intel_held[1] == 0) {
        send_move_object(server, 1, 1, server->protocol.gamemode.intel[1]);
    }
}

static void receive_block_action(server_t* server, uint8_t playerID, datastream_t* data)
{
    uint8_t ID = datastream_read_u8(data);
    if (playerID != ID) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in block action packet", playerID, ID);
    }
    if (server->player[playerID].can_build && server->global_ab) {
        uint8_t  actionType = datastream_read_u8(data);
        uint32_t X          = datastream_read_u32(data);
        uint32_t Y          = datastream_read_u32(data);
        uint32_t Z          = datastream_read_u32(data);
        if (server->player[playerID].sprinting) {
            return;
        }
        vector3i_t vectorBlock  = {X, Y, Z};
        vector3f_t vectorfBlock = {(float) X, (float) Y, (float) Z};
        vector3f_t playerVector = server->player[playerID].movement.position;
        if (((server->player[playerID].item == 0 && (actionType == 1 || actionType == 2)) ||
             (server->player[playerID].item == 1 && actionType == 0) ||
             (server->player[playerID].item == 2 && actionType == 1)))
        {
            if ((DistanceIn3D(vectorfBlock, playerVector) <= 4 || server->player[playerID].item == 2) &&
                valid_pos_v3i(server, vectorBlock))
            {
                switch (actionType) {
                    case 0:
                    {
                        if (gamemode_block_checks(server, X, Y, Z)) {
                            uint64_t timeNow = get_nanos();
                            if (server->player[playerID].blocks > 0 &&
                                diffIsOlderThen(
                                timeNow, &server->player[playerID].timers.since_last_block_plac, BLOCK_DELAY) &&
                                diffIsOlderThenDontUpdate(
                                timeNow, server->player[playerID].timers.since_last_block_dest, BLOCK_DELAY) &&
                                diffIsOlderThenDontUpdate(
                                timeNow, server->player[playerID].timers.since_last_3block_dest, BLOCK_DELAY))
                            {
                                mapvxl_set_color(&server->s_map.map, X, Y, Z, server->player[playerID].tool_color.raw);
                                server->player[playerID].blocks--;
                                moveIntelAndTentUp(server);
                                send_block_action(server, playerID, actionType, X, Y, Z);
                            }
                        }
                    } break;

                    case 1:
                    {
                        if (Z < 62 && gamemode_block_checks(server, X, Y, Z)) {
                            uint64_t timeNow = get_nanos();
                            if ((diffIsOlderThen(
                                 timeNow, &server->player[playerID].timers.since_last_block_dest, SPADE_DELAY) &&
                                 diffIsOlderThenDontUpdate(
                                 timeNow, server->player[playerID].timers.since_last_3block_dest, SPADE_DELAY) &&
                                 diffIsOlderThenDontUpdate(
                                 timeNow, server->player[playerID].timers.since_last_block_plac, SPADE_DELAY)) ||
                                server->player[playerID].item == 2)
                            {
                                if (server->player[playerID].item == 2) {
                                    if (server->player[playerID].weapon_clip <= 0) {
                                        sendMessageToStaff(server,
                                                           "Player %s (#%hhu) probably has hack to have more ammo",
                                                           server->player[playerID].name,
                                                           playerID);
                                        return;
                                    } else if (server->player[playerID].weapon == 0 &&
                                               diffIsOlderThen(
                                               timeNow,
                                               &server->player[playerID].timers.since_last_block_dest_with_gun,
                                               ((RIFLE_DELAY * 2) - (NANO_IN_MILLI * 10))) == 0)
                                    {
                                        sendMessageToStaff(server,
                                                           "Player %s (#%hhu) probably has rapid shooting hack",
                                                           server->player[playerID].name,
                                                           playerID);
                                        return;
                                    } else if (server->player[playerID].weapon == 1 &&
                                               diffIsOlderThen(
                                               timeNow,
                                               &server->player[playerID].timers.since_last_block_dest_with_gun,
                                               ((SMG_DELAY * 3) - (NANO_IN_MILLI * 10))) == 0)
                                    {
                                        sendMessageToStaff(server,
                                                           "Player %s (#%hhu) probably has rapid shooting hack",
                                                           server->player[playerID].name,
                                                           playerID);
                                        return;
                                    } else if (server->player[playerID].weapon == 2 &&
                                               diffIsOlderThen(
                                               timeNow,
                                               &server->player[playerID].timers.since_last_block_dest_with_gun,
                                               (SHOTGUN_DELAY - (NANO_IN_MILLI * 10))) == 0)
                                    {
                                        sendMessageToStaff(server,
                                                           "Player %s (#%hhu) probably has rapid shooting hack",
                                                           server->player[playerID].name,
                                                           playerID);
                                        return;
                                    }
                                }

                                vector3i_t  position = {X, Y, Z};
                                vector3i_t* neigh    = get_neighbours(position);
                                mapvxl_set_air(&server->s_map.map, position.x, position.y, position.z);
                                for (int i = 0; i < 6; ++i) {
                                    if (neigh[i].z < 62) {
                                        check_node(server, neigh[i]);
                                    }
                                }
                                if (server->player[playerID].item != 2) {
                                    if (server->player[playerID].blocks < 50) {
                                        server->player[playerID].blocks++;
                                    }
                                }
                                send_block_action(server, playerID, actionType, X, Y, Z);
                            }
                        }
                    } break;

                    case 2:
                    {
                        if (gamemode_block_checks(server, X, Y, Z) && gamemode_block_checks(server, X, Y, Z + 1) &&
                            gamemode_block_checks(server, X, Y, Z - 1))
                        {
                            uint64_t timeNow = get_nanos();
                            if (diffIsOlderThen(
                                timeNow, &server->player[playerID].timers.since_last_3block_dest, THREEBLOCK_DELAY) &&
                                diffIsOlderThenDontUpdate(
                                timeNow, server->player[playerID].timers.since_last_block_dest, THREEBLOCK_DELAY) &&
                                diffIsOlderThenDontUpdate(
                                timeNow, server->player[playerID].timers.since_last_block_plac, THREEBLOCK_DELAY))
                            {
                                for (uint32_t z = Z - 1; z <= Z + 1; z++) {
                                    if (z < 62) {
                                        mapvxl_set_air(&server->s_map.map, X, Y, z);
                                        vector3i_t  position = {X, Y, z};
                                        vector3i_t* neigh    = get_neighbours(position);
                                        mapvxl_set_air(&server->s_map.map, position.x, position.y, position.z);
                                        for (int i = 0; i < 6; ++i) {
                                            if (neigh[i].z < 62) {
                                                check_node(server, neigh[i]);
                                            }
                                        }
                                    }
                                }
                                send_block_action(server, playerID, actionType, X, Y, Z);
                            }
                        }
                    } break;
                }
                moveIntelAndTentDown(server);
            }
        } else {
            LOG_WARNING("Player %s (#%hhu) may be using BlockExploit with Item: %d and Action: %d",
                        server->player[playerID].name,
                        playerID,
                        server->player[playerID].item,
                        actionType);
        }
    }
}

static void receive_block_line(server_t* server, uint8_t playerID, datastream_t* data)
{
    uint8_t ID = datastream_read_u8(data);
    if (playerID != ID) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in blockline packet", playerID, ID);
    }
    uint64_t timeNow = get_nanos();
    if (server->player[playerID].blocks > 0 && server->player[playerID].can_build && server->global_ab &&
        server->player[playerID].item == 1 &&
        diffIsOlderThen(timeNow, &server->player[playerID].timers.since_last_block_plac, BLOCK_DELAY) &&
        diffIsOlderThenDontUpdate(timeNow, server->player[playerID].timers.since_last_block_dest, BLOCK_DELAY) &&
        diffIsOlderThenDontUpdate(timeNow, server->player[playerID].timers.since_last_3block_dest, BLOCK_DELAY))
    {
        vector3i_t start;
        vector3i_t end;
        start.x = datastream_read_u32(data);
        start.y = datastream_read_u32(data);
        start.z = datastream_read_u32(data);
        end.x   = datastream_read_u32(data);
        end.y   = datastream_read_u32(data);
        end.z   = datastream_read_u32(data);
        if (server->player[playerID].sprinting) {
            return;
        }
        vector3f_t startF = {start.x, start.y, start.z};
        vector3f_t endF   = {end.x, end.y, end.z};
        if (DistanceIn3D(endF, server->player[playerID].movement.position) <= 4 &&
            DistanceIn3D(startF, server->player[playerID].locAtClick) <= 4 && valid_pos_v3f(server, startF) &&
            valid_pos_v3f(server, endF))
        {
            int size = line_get_blocks(&start, &end, server->s_map.result_line);
            server->player[playerID].blocks -= size;
            for (int i = 0; i < size; i++) {
                mapvxl_set_color(&server->s_map.map,
                                 server->s_map.result_line[i].x,
                                 server->s_map.result_line[i].y,
                                 server->s_map.result_line[i].z,
                                 server->player[playerID].tool_color.raw);
            }
            moveIntelAndTentUp(server);
            send_block_line(server, playerID, start, end);
        }
    }
}

static void receive_set_tool(server_t* server, uint8_t playerID, datastream_t* data)
{
    uint8_t ID   = datastream_read_u8(data);
    uint8_t tool = datastream_read_u8(data);
    if (server->player[playerID].item == tool) {
        return;
    }
    if (playerID != ID) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in set tool packet", playerID, ID);
    }

    server->player[playerID].item      = tool;
    server->player[playerID].reloading = 0;
    send_set_tool(server, playerID, tool);
}

static void receive_set_color(server_t* server, uint8_t playerID, datastream_t* data)
{
    uint8_t ID = datastream_read_u8(data);
    uint8_t B  = datastream_read_u8(data);
    uint8_t G  = datastream_read_u8(data);
    uint8_t R  = datastream_read_u8(data);
    uint8_t A  = 0;

    if (playerID != ID) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in set color packet", playerID, ID);
    }

    server->player[playerID].tool_color.a = A;
    server->player[playerID].tool_color.r = R;
    server->player[playerID].tool_color.g = G;
    server->player[playerID].tool_color.b = B;
    send_set_color(server, playerID, R, G, B);
}

static void receive_weapon_input(server_t* server, uint8_t playerID, datastream_t* data)
{
    uint8_t mask   = 1;
    uint8_t ID     = datastream_read_u8(data);
    uint8_t wInput = datastream_read_u8(data);
    if (playerID != ID) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in weapon input packet", playerID, ID);
    } else if (server->player[playerID].state != STATE_READY) {
        return;
    } else if (server->player[playerID].sprinting) {
        wInput = 0; /* Do not return just set it to 0 as we want to send to players that the player is no longer
                    shooting when they start sprinting */
    }
    server->player[playerID].primary_fire   = wInput & mask;
    server->player[playerID].secondary_fire = (wInput >> 1) & mask;

    if (server->player[playerID].secondary_fire && server->player[playerID].item == 1) {
        server->player[playerID].locAtClick = server->player[playerID].movement.position;
    } else if (server->player[playerID].secondary_fire && server->player[playerID].item == 0) {
        server->player[playerID].timers.since_possible_spade_nade = get_nanos();
    }

    else if (server->player[playerID].weapon_clip > 0)
    {
        send_weapon_input(server, playerID, wInput);
        uint64_t timeDiff = 0;
        switch (server->player[playerID].weapon) {
            case WEAPON_RIFLE:
            {
                timeDiff = NANO_IN_MILLI * 500;
                break;
            }
            case WEAPON_SMG:
            {
                timeDiff = NANO_IN_MILLI * 100;
                break;
            }
            case WEAPON_SHOTGUN:
            {
                timeDiff = NANO_IN_MILLI * 1000;
                break;
            }
        }

        if (server->player[playerID].primary_fire &&
            diffIsOlderThen(get_nanos(), &server->player[playerID].timers.since_last_weapon_input, timeDiff))
        {
            server->player[playerID].timers.since_last_weapon_input = get_nanos();
            server->player[playerID].weapon_clip--;
            server->player[playerID].reloading = 0;
            if ((server->player[playerID].movement.previous_orientation.x ==
                 server->player[playerID].movement.forward_orientation.x) &&
                (server->player[playerID].movement.previous_orientation.y ==
                 server->player[playerID].movement.forward_orientation.y) &&
                (server->player[playerID].movement.previous_orientation.z ==
                 server->player[playerID].movement.forward_orientation.z) &&
                server->player[playerID].item == TOOL_GUN)
            {
                for (int i = 0; i < server->protocol.max_players; ++i) {
                    if (is_past_join_screen(server, i) && isStaff(server, i)) {
                        char message[200];
                        snprintf(message, 200, "WARNING. Player %d may be using no recoil", playerID);
                        send_server_notice(server, i, 0, message);
                    }
                }
            }
            server->player[playerID].movement.previous_orientation =
            server->player[playerID].movement.forward_orientation;
        }
    } else {
        // sendKillActionPacket(server, playerID, playerID, 0, 30, 0);
    }
}

static void receive_weapon_reload(server_t* server, uint8_t playerID, datastream_t* data)
{
    uint8_t ID      = datastream_read_u8(data);
    uint8_t clip    = datastream_read_u8(data);
    uint8_t reserve = datastream_read_u8(data);
    if (playerID != ID) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in weapon reload packet", playerID, ID);
    }
    server->player[playerID].primary_fire   = 0;
    server->player[playerID].secondary_fire = 0;

    if (server->player[playerID].weapon_reserve == 0 || server->player[playerID].reloading) {
        return;
    }
    server->player[playerID].reloading                 = 1;
    server->player[playerID].timers.since_reload_start = get_nanos();
    send_weapon_reload(server, playerID, 1, clip, reserve);
}

static void receive_change_team(server_t* server, uint8_t playerID, datastream_t* data)
{
    uint8_t ID = datastream_read_u8(data);
    server->protocol.num_team_users[server->player[playerID].team]--;
    uint8_t team = datastream_read_u8(data);

    if (team != 0 && team != 1 && team != 255) {
        LOG_WARNING(
        "Player %s (#%hhu) sent invalid team. Switching them to Spectator", server->player[playerID].name, playerID);
        team = 255;
    }

    server->player[playerID].team = team;
    if (playerID != ID) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in change team packet", playerID, ID);
    }
    server->protocol.num_team_users[server->player[playerID].team]++;
    send_kill_action_packet(server, playerID, playerID, 5, 5, 0);
    server->player[playerID].state = STATE_WAITING_FOR_RESPAWN;
}

static void receive_change_weapon(server_t* server, uint8_t playerID, datastream_t* data)
{
    uint8_t ID     = datastream_read_u8(data);
    uint8_t weapon = datastream_read_u8(data);
    if (server->player[playerID].weapon == weapon) {
        return;
    }
    server->player[playerID].weapon = weapon;
    if (playerID != ID) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in change weapon packet", playerID, ID);
    }
    send_kill_action_packet(server, playerID, playerID, 6, 5, 0);
    server->player[playerID].state = STATE_WAITING_FOR_RESPAWN;
}

static void receive_version_response(server_t* server, uint8_t playerID, datastream_t* data)
{
    server->player[playerID].client           = datastream_read_u8(data);
    server->player[playerID].version_major    = datastream_read_u8(data);
    server->player[playerID].version_minor    = datastream_read_u8(data);
    server->player[playerID].version_revision = datastream_read_u8(data);
    uint32_t length                           = datastream_left(data);
    if (length < 256) {
        server->player[playerID].os_info[length] = '\0';
        datastream_read_array(data, server->player[playerID].os_info, length);
    } else {
        snprintf(server->player[playerID].os_info, 8, "Unknown");
    }
    if (server->player[playerID].client == 'o') {
        if (!(server->player[playerID].version_major == 0 && server->player[playerID].version_minor == 1 &&
              (server->player[playerID].version_revision == 3 || server->player[playerID].version_revision == 5)))
        {
            enet_peer_disconnect(server->player[playerID].peer, REASON_KICKED);
        }
    } else if (server->player[playerID].client == 'B') {
        if (!(server->player[playerID].version_major == 0 && server->player[playerID].version_minor == 1 &&
              server->player[playerID].version_revision == 5))
        {
            enet_peer_disconnect(server->player[playerID].peer, REASON_KICKED);
        }
    }
}

void init_packets(server_t* server)
{
    server->packets                = NULL;
    packet_manager_t packets[] = {{0, &receive_position_data},
                                      {1, &receive_orientation_data},
                                      {3, &receive_input_data},
                                      {4, &receive_weapon_input},
                                      {5, &receive_hit_packet},
                                      {6, &receive_grenade_packet},
                                      {7, &receive_set_tool},
                                      {8, &receive_set_color},
                                      {9, &receive_existing_player},
                                      {13, &receive_block_action},
                                      {14, &receive_block_line},
                                      {17, &receive_handle_send_message},
                                      {28, &receive_weapon_reload},
                                      {29, &receive_change_team},
                                      {30, &receive_change_weapon},
                                      {34, &receive_version_response}};
    for (unsigned long i = 0; i < sizeof(packets) / sizeof(packet_manager_t); i++) {
        packet_t* packet  = malloc(sizeof(packet_t));
        packet->id = packets[i].id;
        packet->packet    = packets[i].packet;
        HASH_ADD_INT(server->packets, id, packet);
        packet_t *pp;
        int d = 13;
        HASH_FIND_INT(server->packets, &d, pp);
        if (pp == NULL) {
            LOG_DEBUG("FUCK");
        }
        else {
            LOG_STATUS("%d", pp->id);
        }
    }
}

void free_all_packets(server_t* server)
{
    packet_t* current_packet;
    packet_t* tmp;

    HASH_ITER(hh, server->packets, current_packet, tmp)
    {
        HASH_DEL(server->packets, current_packet);
        free(current_packet);
    }
}

void on_packet_received(server_t* server, uint8_t playerID, datastream_t* data)
{
    uint8_t   type = datastream_read_u8(data);
    packet_t* packet_p = NULL;
    int type_int = (int)type;
    HASH_FIND_INT(server->packets, &type_int, packet_p);
    if (packet_p == NULL) {
        LOG_WARNING("Unknown packet with ID %d received", type_int);
        return;
    }
    else {
        LOG_STATUS("It works fine");
    }
    //(void)playerID;
    //(void)data;
    packet_p->packet(server, playerID, data);
}
