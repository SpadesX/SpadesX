// Copyright DarkNeutrino 2021
#include "../Extern/libmapvxl/libmapvxl.h"
#include "Commands.h"
#include "Events.h"
#include "ParseConvert.h"
#include "Protocol.h"
#include "Structs.h"
#include "Util/Compress.h"
#include "Util/DataStream.h"
#include "Util/Enums.h"
#include "Util/Line.h"
#include "Util/Log.h"
#include "Util/Physics.h"
#include "Util/Queue.h"
#include "Util/Types.h"
#include "Util/Uthash.h"
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

inline static uint8_t allow_shot(server_t*  server,
                                 uint8_t    player_id,
                                 uint8_t    player_hit_id,
                                 uint64_t   time_now,
                                 float      distance,
                                 long*      x,
                                 long*      y,
                                 long*      z,
                                 vector3f_t shot_pos,
                                 vector3f_t shot_orien,
                                 vector3f_t hit_pos,
                                 vector3f_t shot_eye_pos)
{
    uint8_t   ret        = 0;
    player_t* player     = &server->player[player_id];
    player_t* player_hit = &server->player[player_hit_id];
    if (player->primary_fire &&
        ((player->item == 0 && diff_is_older_then(time_now, &player->timers.since_last_shot, NANO_IN_MILLI * 100)) ||
         (player->item == 2 && player->weapon == 0 &&
          diff_is_older_then(time_now, &player->timers.since_last_shot, NANO_IN_MILLI * 500)) ||
         (player->item == 2 && player->weapon == 1 &&
          diff_is_older_then(time_now, &player->timers.since_last_shot, NANO_IN_MILLI * 110)) ||
         (player->item == 2 && player->weapon == 2 &&
          diff_is_older_then(time_now, &player->timers.since_last_shot, NANO_IN_MILLI * 1000))) &&
        player->alive && player_hit->alive && (player->team != player_hit->team || player->allow_team_killing) &&
        (player->allow_killing && server->global_ak) && physics_validate_hit(shot_pos, shot_orien, hit_pos, 5) &&
        (physics_cast_ray(server,
                          shot_eye_pos.x,
                          shot_eye_pos.y,
                          shot_eye_pos.z,
                          shot_orien.x,
                          shot_orien.y,
                          shot_orien.z,
                          distance,
                          x,
                          y,
                          z) == 0 ||
         physics_cast_ray(server,
                          shot_eye_pos.x,
                          shot_eye_pos.y,
                          shot_eye_pos.z - 1,
                          shot_orien.x,
                          shot_orien.y,
                          shot_orien.z,
                          distance,
                          x,
                          y,
                          z) == 0))
    {
        ret = 1;
    }
    return ret;
}

void send_restock(server_t* server, uint8_t player_id)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 2, ENET_PACKET_FLAG_RELIABLE);
    player_t*   player = &server->player[player_id];
    stream_t    stream;
    stream_from_enet_packet(&stream, packet);
    stream_write_u8(&stream, PACKET_TYPE_RESTOCK);
    stream_write_u8(&stream, player_id);
    if (enet_peer_send(player->peer, 0, packet) != 0) {
        enet_packet_destroy(packet);
    }
}

void send_move_object(server_t* server, uint8_t object, uint8_t team, vector3f_t pos)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 15, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream;
    stream_from_enet_packet(&stream, packet);
    stream_write_u8(&stream, PACKET_TYPE_MOVE_OBJECT);
    stream_write_u8(&stream, object);
    stream_write_u8(&stream, team);
    stream_write_f(&stream, pos.x);
    stream_write_f(&stream, pos.y);
    stream_write_f(&stream, pos.z);

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

void send_intel_capture(server_t* server, uint8_t player_id, uint8_t winning)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    uint8_t   team;
    player_t* player = &server->player[player_id];
    if (player->team == 0) {
        team = 1;
    } else {
        team = 0;
    }
    if (player->has_intel == 0 || server->protocol.gamemode.intel_held[team] == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 3, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream;
    stream_from_enet_packet(&stream, packet);
    stream_write_u8(&stream, PACKET_TYPE_INTEL_CAPTURE);
    stream_write_u8(&stream, player_id);
    stream_write_u8(&stream, winning);
    player->has_intel                          = 0;
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

void send_intel_pickup(server_t* server, uint8_t player_id)
{
    if (server->protocol.num_players == 0) {
        return;
    }

    uint8_t   team;
    player_t* player = &server->player[player_id];

    if (player->team == 0) {
        team = 1;
    } else {
        team = 0;
    }

    if (player->has_intel == 1 || server->protocol.gamemode.intel_held[team] == 1) {
        return;
    }

    ENetPacket* packet = enet_packet_create(NULL, 2, ENET_PACKET_FLAG_RELIABLE);

    stream_t stream;
    stream_from_enet_packet(&stream, packet);
    stream_write_u8(&stream, PACKET_TYPE_INTEL_PICKUP);
    stream_write_u8(&stream, player_id);
    player->has_intel                                         = 1;
    server->protocol.gamemode.player_intel_team[player->team] = player_id;
    server->protocol.gamemode.intel_held[team]                = 1;

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

void send_intel_drop(server_t* server, uint8_t player_id)
{
    if (server->protocol.num_players == 0) {
        return;
    }

    uint8_t   team;
    player_t* player = &server->player[player_id];

    if (player->team == TEAM_A) {
        team = TEAM_B;
    } else {
        team = TEAM_A;
    }

    if (player->has_intel == 0 || server->protocol.gamemode.intel_held[team] == 0) {
        return;
    }

    ENetPacket* packet = enet_packet_create(NULL, 14, ENET_PACKET_FLAG_RELIABLE);

    stream_t stream;
    stream_from_enet_packet(&stream, packet);
    stream_write_u8(&stream, PACKET_TYPE_INTEL_DROP);
    stream_write_u8(&stream, player_id);

    gamemode_vars_t* gamemode = &server->protocol.gamemode;
    if (server->protocol.current_gamemode == GAME_MODE_BABEL) {
        uint16_t top_block =
        mapvxl_find_top_block(&server->s_map.map, server->s_map.map.size_x / 2, server->s_map.map.size_y / 2);

        stream_write_f(&stream, (float) server->s_map.map.size_x / 2);
        stream_write_f(&stream, (float) server->s_map.map.size_y / 2);
        stream_write_f(&stream, top_block);

        gamemode->intel[team].x       = (float) server->s_map.map.size_x / 2;
        gamemode->intel[team].y       = (float) server->s_map.map.size_y / 2;
        gamemode->intel[team].z       = top_block;
        gamemode->intel[player->team] = gamemode->intel[team];
        send_move_object(server, player->team, player->team, gamemode->intel[team]);
    } else {
        vector3f_t* position = &player->movement.position;
        stream_write_f(&stream, position->x);
        stream_write_f(&stream, position->y);
        stream_write_f(&stream, (float) mapvxl_find_top_block(&server->s_map.map, position->x, position->y));

        gamemode->intel[team].x = (int) position->x;
        gamemode->intel[team].y = (int) position->y;
        gamemode->intel[team].z = mapvxl_find_top_block(&server->s_map.map, position->x, position->y);
    }

    player->has_intel                         = 0;
    gamemode->player_intel_team[player->team] = 32;
    gamemode->intel_held[team]                = 0;

    LOG_INFO("Dropping intel at X: %d, Y: %d, Z: %d",
             (int) gamemode->intel[team].x,
             (int) gamemode->intel[team].y,
             (int) gamemode->intel[team].z);

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

void send_grenade(server_t* server, uint8_t player_id, float fuse, vector3f_t position, vector3f_t velocity)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 30, ENET_PACKET_FLAG_RELIABLE);

    stream_t stream;
    stream_from_enet_packet(&stream, packet);
    stream_write_u8(&stream, PACKET_TYPE_GRENADE_PACKET);
    stream_write_u8(&stream, player_id);
    stream_write_f(&stream, fuse);
    stream_write_f(&stream, position.x);
    stream_write_f(&stream, position.y);
    stream_write_f(&stream, position.z);
    stream_write_f(&stream, velocity.x);
    stream_write_f(&stream, velocity.y);
    stream_write_f(&stream, velocity.z);
    if (send_packet_except_sender(server, packet, player_id) == 0) {
        enet_packet_destroy(packet);
    }
}

void send_player_left(server_t* server, uint8_t player_id)
{
    char      ipString[17];
    player_t* player = &server->player[player_id];
    format_ip_to_str(ipString, player->ip);
    LOG_INFO("Player %s (%s, #%hhu) disconnected", player->name, ipString, player_id);
    if (server->protocol.num_players == 0) {
        return;
    }
    for (uint8_t i = 0; i < server->protocol.max_players; ++i) {
        if (i != player_id && is_past_state_data(server, i)) {
            ENetPacket* packet = enet_packet_create(NULL, 2, ENET_PACKET_FLAG_RELIABLE);

            stream_t stream;
            stream_from_enet_packet(&stream, packet);
            stream_write_u8(&stream, PACKET_TYPE_PLAYER_LEFT);
            stream_write_u8(&stream, player_id);

            if (enet_peer_send(server->player[i].peer, 0, packet) != 0) {
                LOG_WARNING("Failed to send player left event");
                enet_packet_destroy(packet);
            }
        }
    }
}

void send_weapon_reload(server_t* server, uint8_t player_id, uint8_t startAnimation, uint8_t clip, uint8_t reserve)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 4, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    player_t*   player = &server->player[player_id];
    stream_write_u8(&stream, PACKET_TYPE_WEAPON_RELOAD);
    stream_write_u8(&stream, player_id);
    if (startAnimation) {
        stream_write_u8(&stream, clip);
        stream_write_u8(&stream, reserve);
    } else {
        stream_write_u8(&stream, player->weapon_clip);
        stream_write_u8(&stream, player->weapon_reserve);
    }
    if (startAnimation) {
        uint8_t sendSucc = 0;
        for (int player = 0; player < server->protocol.max_players; ++player) {
            if (is_past_state_data(server, player) && player != player_id) {
                if (enet_peer_send(server->player[player].peer, 0, packet) == 0) {
                    sendSucc = 1;
                }
            }
        }
        if (sendSucc == 0) {
            enet_packet_destroy(packet);
        }
    } else {
        if (is_past_join_screen(server, player_id)) {
            if (enet_peer_send(player->peer, 0, packet) != 0) {
                enet_packet_destroy(packet);
            }
        }
    }
}

void send_weapon_input(server_t* server, uint8_t player_id, uint8_t wInput)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 3, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    player_t*   player = &server->player[player_id];
    stream_write_u8(&stream, PACKET_TYPE_WEAPON_INPUT);
    stream_write_u8(&stream, player_id);
    if (player->sprinting) {
        wInput = 0;
    }
    stream_write_u8(&stream, wInput);
    if (send_packet_except_sender(server, packet, player_id) == 0) {
        enet_packet_destroy(packet);
    }
}

void send_set_color(server_t* server, uint8_t player_id, color_t color)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 5, ENET_PACKET_FLAG_RELIABLE);

    stream_t stream;
    stream_from_enet_packet(&stream, packet);
    stream_write_u8(&stream, PACKET_TYPE_SET_COLOR);
    stream_write_u8(&stream, player_id);
    stream_write_u8(&stream, color.b);
    stream_write_u8(&stream, color.g);
    stream_write_u8(&stream, color.r);
    if (send_packet_except_sender(server, packet, player_id) == 0) {
        enet_packet_destroy(packet);
    }
}

void send_set_color_to_player(server_t* server, uint8_t player_id, uint8_t send_to_id, uint8_t R, uint8_t G, uint8_t B)
{
    if (server->protocol.num_players == 0) {
        return;
    }

    ENetPacket* packet = enet_packet_create(NULL, 5, ENET_PACKET_FLAG_RELIABLE);

    stream_t stream;
    stream_from_enet_packet(&stream, packet);
    stream_write_u8(&stream, PACKET_TYPE_SET_COLOR);
    stream_write_u8(&stream, player_id);
    stream_write_u8(&stream, B);
    stream_write_u8(&stream, G);
    stream_write_u8(&stream, R);

    if (enet_peer_send(server->player[send_to_id].peer, 0, packet) != 0) {
        enet_packet_destroy(packet);
    }
}

void send_set_tool(server_t* server, uint8_t player_id, uint8_t tool)
{
    if (server->protocol.num_players == 0) {
        return;
    }

    ENetPacket* packet = enet_packet_create(NULL, 3, ENET_PACKET_FLAG_RELIABLE);

    stream_t stream;
    stream_from_enet_packet(&stream, packet);
    stream_write_u8(&stream, PACKET_TYPE_SET_TOOL);
    stream_write_u8(&stream, player_id);
    stream_write_u8(&stream, tool);

    if (send_packet_except_sender(server, packet, player_id) == 0) {
        enet_packet_destroy(packet);
    }
}

void send_block_line(server_t* server, uint8_t player_id, vector3i_t start, vector3i_t end)
{
    if (server->protocol.num_players == 0) {
        return;
    }

    ENetPacket* packet = enet_packet_create(NULL, 26, ENET_PACKET_FLAG_RELIABLE);

    stream_t stream;
    stream_from_enet_packet(&stream, packet);
    stream_write_u8(&stream, PACKET_TYPE_BLOCK_LINE);
    stream_write_u8(&stream, player_id);
    stream_write_u32(&stream, start.x);
    stream_write_u32(&stream, start.y);
    stream_write_u32(&stream, start.z);
    stream_write_u32(&stream, end.x);
    stream_write_u32(&stream, end.y);
    stream_write_u32(&stream, end.z);

    uint8_t sent = 0;
    for (int id = 0; id < server->protocol.max_players; ++id) {
        player_t* player = &server->player[id];

        if (is_past_state_data(server, id)) {
            if (enet_peer_send(player->peer, 0, packet) == 0) {
                sent = 1;
            }
        } else if (player->state == STATE_STARTING_MAP || player->state == STATE_LOADING_CHUNKS) {
            block_node_t* node   = (block_node_t*) malloc(sizeof(*node));
            node->position.x     = start.x;
            node->position.y     = start.y;
            node->position.z     = start.z;
            node->position_end.x = end.x;
            node->position_end.y = end.y;
            node->position_end.z = end.z;
            node->color          = player->color;
            node->type           = 10;
            node->sender_id      = player_id;
            LL_APPEND(player->blockBuffer, node);
        }
    }

    if (sent == 0) {
        enet_packet_destroy(packet);
    }
}

void block_line_to_player(server_t* server, uint8_t player_id, uint8_t send_to_id, vector3i_t start, vector3i_t end)
{
    if (server->protocol.num_players == 0) {
        return;
    }

    ENetPacket* packet = enet_packet_create(NULL, 26, ENET_PACKET_FLAG_RELIABLE);

    stream_t stream;
    stream_from_enet_packet(&stream, packet);
    stream_write_u8(&stream, PACKET_TYPE_BLOCK_LINE);
    stream_write_u8(&stream, player_id);
    stream_write_u32(&stream, start.x);
    stream_write_u32(&stream, start.y);
    stream_write_u32(&stream, start.z);
    stream_write_u32(&stream, end.x);
    stream_write_u32(&stream, end.y);
    stream_write_u32(&stream, end.z);
    uint8_t sent = 0;
    if (enet_peer_send(server->player[send_to_id].peer, 0, packet) == 0) {
        sent = 1;
    }

    if (sent == 0) {
        enet_packet_destroy(packet);
    }
}

void send_block_action(server_t* server, uint8_t player_id, uint8_t action_type, int X, int Y, int Z)
{
    if (server->protocol.num_players == 0) {
        return;
    }

    ENetPacket* packet = enet_packet_create(NULL, 15, ENET_PACKET_FLAG_RELIABLE);

    stream_t stream;
    stream_from_enet_packet(&stream, packet);
    stream_write_u8(&stream, PACKET_TYPE_BLOCK_ACTION);
    stream_write_u8(&stream, player_id);
    stream_write_u8(&stream, action_type);
    stream_write_u32(&stream, X);
    stream_write_u32(&stream, Y);
    stream_write_u32(&stream, Z);

    uint8_t sent = 0;
    for (int id = 0; id < server->protocol.max_players; ++id) {
        player_t* player = &server->player[id];
        if (is_past_state_data(server, id)) {
            if (enet_peer_send(player->peer, 0, packet) == 0) {
                sent = 1;
            }
        } else if (player->state == STATE_STARTING_MAP || player->state == STATE_LOADING_CHUNKS) {
            block_node_t* node = (block_node_t*) malloc(sizeof(*node));
            node->position.x   = X;
            node->position.y   = Y;
            node->position.z   = Z;
            node->color        = player->color;
            node->type         = action_type;
            node->sender_id    = player_id;
            LL_APPEND(player->blockBuffer, node);
        }
    }
    if (sent == 0) {
        enet_packet_destroy(packet);
    }
}
void send_block_action_to_player(server_t* server,
                                 uint8_t   player_id,
                                 uint8_t   send_to_id,
                                 uint8_t   action_type,
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
    stream_write_u8(&stream, player_id);
    stream_write_u8(&stream, action_type);
    stream_write_u32(&stream, X);
    stream_write_u32(&stream, Y);
    stream_write_u32(&stream, Z);
    uint8_t sent = 0;
    if (enet_peer_send(server->player[send_to_id].peer, 0, packet) == 0) {
        sent = 1;
    }
    if (sent == 0) {
        enet_packet_destroy(packet);
    }
}

void send_state_data(server_t* server, uint8_t player_id)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 104, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    player_t*   player = &server->player[player_id];
    stream_write_u8(&stream, PACKET_TYPE_STATE_DATA);
    stream_write_u8(&stream, player_id);
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

void send_input_data(server_t* server, uint8_t player_id)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 3, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    player_t*   player = &server->player[player_id];
    stream_write_u8(&stream, PACKET_TYPE_INPUT_DATA);
    stream_write_u8(&stream, player_id);
    stream_write_u8(&stream, player->input);
    if (send_packet_except_sender_dist_check(server, packet, player_id) == 0) {
        enet_packet_destroy(packet);
    }
}

void send_kill_action_packet(server_t* server,
                             uint8_t   killerID,
                             uint8_t   player_id,
                             uint8_t   killReason,
                             uint8_t   respawnTime,
                             uint8_t   makeInvisible)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    player_t* player = &server->player[player_id];
    if (player->alive == 0) {
        return; // Cant kill player if they are dead
    }
    ENetPacket* packet = enet_packet_create(NULL, 5, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    stream_write_u8(&stream, PACKET_TYPE_KILL_ACTION);
    stream_write_u8(&stream, player_id);   // Player that died.
    stream_write_u8(&stream, killerID);    // Player that killed.
    stream_write_u8(&stream, killReason);  // Killing reason (1 is headshot)
    stream_write_u8(&stream, respawnTime); // Time before respawn happens
    uint8_t sent = 0;
    for (int player = 0; player < server->protocol.max_players; ++player) {
        uint8_t isPast = is_past_state_data(server, player);
        if ((makeInvisible && player != player_id && isPast) || (isPast && !makeInvisible)) {
            if (enet_peer_send(server->player[player].peer, 0, packet) == 0) {
                sent = 1;
            }
        }
    }
    if (sent == 0) {
        enet_packet_destroy(packet);
        return; // Do not kill the player since sending the packet failed
    }
    if (!makeInvisible && player->is_invisible == 0) {
        if (killerID != player_id) {
            server->player[killerID].kills++;
        }
        player->deaths++;
        player->alive                        = 0;
        player->respawn_time                 = respawnTime;
        player->timers.start_of_respawn_wait = time(NULL);
        player->state                        = STATE_WAITING_FOR_RESPAWN;
        switch (player->weapon) {
            case 0:
                player->weapon_reserve  = 50;
                player->weapon_clip     = 10;
                player->default_clip    = RIFLE_DEFAULT_CLIP;
                player->default_reserve = RIFLE_DEFAULT_RESERVE;
                break;
            case 1:
                player->weapon_reserve  = 120;
                player->weapon_clip     = 30;
                player->default_clip    = SMG_DEFAULT_CLIP;
                player->default_reserve = SMG_DEFAULT_RESERVE;
                break;
            case 2:
                player->weapon_reserve  = 48;
                player->weapon_clip     = 6;
                player->default_clip    = SHOTGUN_DEFAULT_CLIP;
                player->default_reserve = SHOTGUN_DEFAULT_RESERVE;
                break;
        }
    }
    if (player->has_intel) {
        send_intel_drop(server, player_id);
    }
}

void send_set_hp(server_t*  server,
                 uint8_t    player_id,
                 uint8_t    player_hit_id,
                 long       hp_chnage,
                 uint8_t    type_of_damage,
                 uint8_t    kill_reason,
                 uint8_t    respawn_time,
                 uint8_t    is_grenade,
                 vector3f_t position)
{
    player_t* player     = &server->player[player_id];
    player_t* player_hit = &server->player[player_hit_id];
    if (server->protocol.num_players == 0 || player_hit->team == TEAM_SPECTATOR ||
        (!player->allow_team_killing && player->team == player_hit->team && player_id != player_hit_id))
    {
        return;
    }
    if ((player->allow_killing && server->global_ak && player->allow_killing && player->alive) || type_of_damage == 0) {
        if (hp_chnage > player_hit->hp) {
            hp_chnage = player_hit->hp;
        }
        player_hit->hp -= hp_chnage;
        if (player_hit->hp < 0) // We should NEVER return true here. If we do stuff is really broken
            player_hit->hp = 0;

        else if (player_hit->hp > 100) // Same as above
            player_hit->hp = 100;

        if (player_hit->hp == 0) {
            send_kill_action_packet(server, player_id, player_hit_id, kill_reason, respawn_time, 0);
            return;
        }

        else if (player_hit->hp > 0 && player_hit->hp < 100)
        {
            ENetPacket* packet = enet_packet_create(NULL, 15, ENET_PACKET_FLAG_RELIABLE);
            stream_t    stream = {packet->data, packet->dataLength, 0};
            stream_write_u8(&stream, PACKET_TYPE_SET_HP);
            stream_write_u8(&stream, player_hit->hp);
            stream_write_u8(&stream, type_of_damage);
            if (type_of_damage != 0 && is_grenade == 0) {
                stream_write_f(&stream, player->movement.position.x);
                stream_write_f(&stream, player->movement.position.y);
                stream_write_f(&stream, player->movement.position.z);
            } else if (type_of_damage != 0 && is_grenade == 1) {
                stream_write_f(&stream, position.x);
                stream_write_f(&stream, position.y);
                stream_write_f(&stream, position.z);
            } else {
                stream_write_f(&stream, 0);
                stream_write_f(&stream, 0);
                stream_write_f(&stream, 0);
            }
            if (enet_peer_send(player_hit->peer, 0, packet) != 0) {
                enet_packet_destroy(packet);
            }
        }
    }
}

void send_existing_player(server_t* server, uint8_t player_id, uint8_t other_id)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet       = enet_packet_create(NULL, 28, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream       = {packet->data, packet->dataLength, 0};
    player_t*   player       = &server->player[player_id];
    player_t*   player_other = &server->player[other_id];
    stream_write_u8(&stream, PACKET_TYPE_EXISTING_PLAYER);
    stream_write_u8(&stream, other_id);                   // ID
    stream_write_u8(&stream, player_other->team);         // TEAM
    stream_write_u8(&stream, player_other->weapon);       // WEAPON
    stream_write_u8(&stream, player_other->item);         // HELD ITEM
    stream_write_u32(&stream, player_other->kills);       // KILLS
    stream_write_color_rgb(&stream, player_other->color); // COLOR
    stream_write_array(&stream, player_other->name, 16);  // NAME

    if (enet_peer_send(player->peer, 0, packet) != 0) {
        LOG_WARNING("Failed to send player state");
        enet_packet_destroy(packet);
    }
}

void send_version_request(server_t* server, uint8_t player_id)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 1, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    player_t*   player = &server->player[player_id];
    stream_write_u8(&stream, PACKET_TYPE_VERSION_REQUEST);
    if (enet_peer_send(player->peer, 0, packet) != 0) {
        enet_packet_destroy(packet);
    }
}
void send_map_start(server_t* server, uint8_t player_id)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    player_t* player = &server->player[player_id];
    LOG_INFO("Sending map info to %s (#%hhu)", player->name, player_id);
    ENetPacket* packet = enet_packet_create(NULL, 5, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    stream_write_u8(&stream, PACKET_TYPE_MAP_START);
    stream_write_u32(&stream, server->s_map.compressed_size);
    if (enet_peer_send(player->peer, 0, packet) == 0) {
        player->state = STATE_LOADING_CHUNKS;

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
        map                          = old_map;
        server->s_map.compressed_map = compress_queue(map, server->s_map.map_size, DEFAULT_COMPRESS_CHUNK_SIZE);
        player->queues               = server->s_map.compressed_map;
        free(map);
    }
}

void send_map_chunks(server_t* server, uint8_t player_id)
{
    player_t* player = &server->player[player_id];
    if (player->queues == NULL) {
        while (server->s_map.compressed_map) {
            server->s_map.compressed_map = queue_pop(server->s_map.compressed_map);
        }
        send_version_request(server, player_id);
        player->state = STATE_JOINING;
        LOG_INFO("Finished sending map to %s (#%hhu)", player->name, player_id);
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

void send_create_player(server_t* server, uint8_t player_id, uint8_t other_id)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet       = enet_packet_create(NULL, 32, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream       = {packet->data, packet->dataLength, 0};
    player_t*   player       = &server->player[player_id];
    player_t*   player_other = &server->player[other_id];
    stream_write_u8(&stream, PACKET_TYPE_CREATE_PLAYER);
    stream_write_u8(&stream, other_id);                              // ID
    stream_write_u8(&stream, player_other->weapon);                  // WEAPON
    stream_write_u8(&stream, player_other->team);                    // TEAM
    stream_write_vector3f(&stream, player_other->movement.position); // X Y Z
    stream_write_array(&stream, player_other->name, 16);             // NAME

    if (enet_peer_send(player->peer, 0, packet) != 0) {
        LOG_WARNING("Failed to send player state");
        enet_packet_destroy(packet);
    }
}

void send_respawn(server_t* server, uint8_t player_id)
{
    player_t* player = &server->player[player_id];
    for (uint8_t i = 0; i < server->protocol.max_players; ++i) {
        if (is_past_join_screen(server, i)) {
            send_create_player(server, i, player_id);
        }
    }
    player->state = STATE_READY;
}

void receive_handle_send_message(server_t* server, uint8_t player_id, stream_t* data)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    uint32_t packet_size = data->length;
    LOG_STATUS("%d %d", packet_size, data->length);
    uint8_t  received_id = stream_read_u8(data);
    int      meant_for   = stream_read_u8(data);
    uint32_t length      = stream_left(data);
    if (length > 2048) {
        length = 2048; // Lets limit messages to 2048 characters
    }
    char* message =
    calloc(length + 1, sizeof(char)); // Allocated 1 more byte in the case that client sent us non null ending string
    stream_read_array(data, message, length);
    if (message[length - 1] != '\0') {
        message[length] = '\0';
        length++;
        packet_size = length + 3;
    }
    if (player_id != received_id) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in message packet", player_id, received_id);
    }
    uint8_t   reset_time = 1;
    player_t* player     = &server->player[player_id];
    if (!diff_is_older_then_dont_update(
        get_nanos(), player->timers.since_last_message_from_spam, (uint64_t) NANO_IN_MILLI * 2000) &&
        !player->muted && player->permissions <= 1)
    {
        player->spam_counter++;
        if (player->spam_counter >= 5) {
            send_message_to_staff(
            server, "WARNING: Player %s (#%d) is trying to spam. Muting.", player->name, player_id);
            send_server_notice(server,
                               player_id,
                               0,
                               "SERVER: You have been muted for excessive spam. If you feel like this is a mistake "
                               "contact staff via /admin command");
            player->muted        = 1;
            player->spam_counter = 0;
        }
        reset_time = 0;
    }
    if (reset_time) {
        player->timers.since_last_message_from_spam = get_nanos();
        player->spam_counter                        = 0;
    }

    if (!diff_is_older_then(get_nanos(), &player->timers.since_last_message, (uint64_t) NANO_IN_MILLI * 400) &&
        player->permissions <= 1)
    {
        send_server_notice(
        server, player_id, 0, "WARNING: You sent last message too fast and thus was not sent out to players");
        return;
    }

    char meantFor[7];
    switch (meant_for) {
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
    LOG_INFO("Player %s (#%hhu) (%s) said: %s", player->name, player_id, meantFor, message);

    uint8_t sent = 0;
    if (message[0] == '/') {
        command_handle(server, player_id, message, 0);
    } else {
        if (!player->muted) {
            ENetPacket* packet = enet_packet_create(NULL, packet_size, ENET_PACKET_FLAG_RELIABLE);
            stream_t    stream = {packet->data, packet->dataLength, 0};
            stream_write_u8(&stream, PACKET_TYPE_CHAT_MESSAGE);
            stream_write_u8(&stream, player_id);
            stream_write_u8(&stream, meant_for);
            stream_write_array(&stream, message, length);
            for (int id = 0; id < server->protocol.max_players; ++id) {
                player_t* player_other = &server->player[id];
                if (is_past_join_screen(server, id) && !player->muted &&
                    ((player_other->team == player->team && meant_for == 1) || meant_for == 0))
                {
                    if (enet_peer_send(player_other->peer, 0, packet) == 0) {
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

void send_world_update(server_t* server, uint8_t player_id)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 1 + (32 * 24), 0);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    player_t*   player = &server->player[player_id];
    stream_write_u8(&stream, PACKET_TYPE_WORLD_UPDATE);

    for (uint8_t j = 0; j < server->protocol.max_players; ++j) {
        if (player_to_player_visible(server, player_id, j) && server->player[j].is_invisible == 0) {
            /*float    dt       = (getNanos() - server->globalTimers.lastUpdateTime) / 1000000000.f;
            Vector3f position = {server->player[j].movement.velocity.x * dt + server->player[j].movement.position.x,
                                 server->player[j].movement.velocity.y * dt + server->player[j].movement.position.y,
                                 server->player[j].movement.velocity.z * dt + server->player[j].movement.position.z};
            WriteVector3f(&stream, position);*/
            stream_write_vector3f(&stream, server->player[j].movement.position);
            stream_write_vector3f(&stream, server->player[j].movement.forward_orientation);
        } else {
            vector3f_t empty;
            empty.x = 0;
            empty.y = 0;
            empty.z = 0;
            stream_write_vector3f(&stream, empty);
            stream_write_vector3f(&stream, empty);
        }
    }
    if (enet_peer_send(player->peer, 0, packet) != 0) {
        enet_packet_destroy(packet);
    }
}

void send_position_packet(server_t* server, uint8_t player_id, float x, float y, float z)
{
    if (server->protocol.num_players == 0) {
        return;
    }
    ENetPacket* packet = enet_packet_create(NULL, 13, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    player_t*   player = &server->player[player_id];
    stream_write_u8(&stream, PACKET_TYPE_POSITION_DATA);
    stream_write_f(&stream, x);
    stream_write_f(&stream, y);
    stream_write_f(&stream, z);
    if (enet_peer_send(player->peer, 0, packet) != 0) {
        enet_packet_destroy(packet);
    }
}

static void receive_grenade_packet(server_t* server, uint8_t player_id, stream_t* data)
{
    uint8_t ID = stream_read_u8(data);
    if (player_id != ID) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in grenade packet", player_id, ID);
    }

    player_t* player = &server->player[player_id];
    if (player->primary_fire && player->item == 1) {
        send_server_notice(server, player_id, 0, "InstaSuicideNade detected. Grenade ineffective");
        send_message_to_staff(server, 0, "Player %s (#%hhu) tried to use InstaSpadeNade", player->name, player_id);
        return;
    }

    uint64_t timeNow = get_nanos();
    if (!diff_is_older_then(timeNow, &player->timers.since_last_grenade_thrown, NANO_IN_MILLI * 500) ||
        !diff_is_older_then_dont_update(timeNow, player->timers.since_possible_spade_nade, (long) NANO_IN_MILLI * 1000))
    {
        return;
    }
    grenade_t* grenade = malloc(sizeof(grenade_t));
    if (player->grenades > 0) {
        grenade->fuse       = stream_read_f(data);
        grenade->position.x = stream_read_f(data);
        grenade->position.y = stream_read_f(data);
        grenade->position.z = stream_read_f(data);
        float velX = stream_read_f(data), velY = stream_read_f(data), velZ = stream_read_f(data);
        if (player->sprinting) {
            free(grenade);
            return;
        }
        grenade->velocity.x = velX;
        grenade->velocity.y = velY;
        grenade->velocity.z = velZ;
        vector3f_t velocity = {grenade->velocity.x - player->movement.velocity.x,
                               grenade->velocity.y - player->movement.velocity.y,
                               grenade->velocity.z - player->movement.velocity.z};
        float      length   = sqrt((velocity.x * velocity.x) + (velocity.y * velocity.y) + (velocity.z * velocity.z));
        if (length > 2) {
            float lenghtNorm = 1 / length;
            velocity.x       = ((velocity.x * lenghtNorm) * 2) + player->movement.velocity.x;
            velocity.y       = ((velocity.y * lenghtNorm) * 2) + player->movement.velocity.y;
            velocity.z       = ((velocity.z * lenghtNorm) * 2) + player->movement.velocity.z;
        }
        vector3f_t posZ1 = grenade->position;
        posZ1.z += 1;
        vector3f_t posZ2 = grenade->position;
        posZ2.z += 2;
        if (valid_pos_v3f(server, grenade->position) ||
            (valid_pos_v3f(server, posZ1) && player->movement.position.z < 0) ||
            (valid_pos_v3f(server, posZ2) && player->movement.position.z < 0))
        {
            send_grenade(server, player_id, grenade->fuse, grenade->position, grenade->velocity);
            grenade->sent            = 1;
            grenade->time_since_sent = get_nanos();
        }
        DL_APPEND(player->grenade, grenade);
        player->grenades--;
    }
}

// player_hit_id is the player that got shot
// player_id is the player who fired.
static void receive_hit_packet(server_t* server, uint8_t player_id, stream_t* data)
{
    player_t*  player        = &server->player[player_id];
    uint8_t    hit_player_id = stream_read_u8(data);
    hit_t      hit_type      = stream_read_u8(data);
    vector3f_t shot_pos      = player->movement.position;
    vector3f_t shot_eye_pos  = player->movement.eye_pos;
    vector3f_t hit_pos       = server->player[hit_player_id].movement.position;
    vector3f_t shot_orien    = player->movement.forward_orientation;
    float      distance      = DistanceIn2D(shot_pos, hit_pos);
    long       x = 0, y = 0, z = 0;

    if (player->sprinting || (player->item == 2 && player->weapon_clip <= 0)) {
        return; // Sprinting and hitting somebody is impossible
    }

    uint64_t timeNow = get_nanos();

    if (allow_shot(
        server, player_id, hit_player_id, timeNow, distance, &x, &y, &z, shot_pos, shot_orien, hit_pos, shot_eye_pos))
    {
        switch (player->weapon) {
            case WEAPON_RIFLE:
            {
                switch (hit_type) {
                    case HIT_TYPE_HEAD:
                    {
                        send_kill_action_packet(server, player_id, hit_player_id, 1, 5, 0);
                        break;
                    }
                    case HIT_TYPE_TORSO:
                    {
                        send_set_hp(server, player_id, hit_player_id, 49, 1, 0, 5, 0, shot_pos);
                        break;
                    }
                    case HIT_TYPE_ARMS:
                    {
                        send_set_hp(server, player_id, hit_player_id, 33, 1, 0, 5, 0, shot_pos);
                        break;
                    }
                    case HIT_TYPE_LEGS:
                    {
                        send_set_hp(server, player_id, hit_player_id, 33, 1, 0, 5, 0, shot_pos);
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
                switch (hit_type) {
                    case HIT_TYPE_HEAD:
                    {
                        send_set_hp(server, player_id, hit_player_id, 75, 1, 1, 5, 0, shot_pos);
                        break;
                    }
                    case HIT_TYPE_TORSO:
                    {
                        send_set_hp(server, player_id, hit_player_id, 29, 1, 0, 5, 0, shot_pos);
                        break;
                    }
                    case HIT_TYPE_ARMS:
                    {
                        send_set_hp(server, player_id, hit_player_id, 18, 1, 0, 5, 0, shot_pos);
                        break;
                    }
                    case HIT_TYPE_LEGS:
                    {
                        send_set_hp(server, player_id, hit_player_id, 18, 1, 0, 5, 0, shot_pos);
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
                switch (hit_type) {
                    case HIT_TYPE_HEAD:
                    {
                        send_set_hp(server, player_id, hit_player_id, 37, 1, 1, 5, 0, shot_pos);
                        break;
                    }
                    case HIT_TYPE_TORSO:
                    {
                        send_set_hp(server, player_id, hit_player_id, 27, 1, 0, 5, 0, shot_pos);
                        break;
                    }
                    case HIT_TYPE_ARMS:
                    {
                        send_set_hp(server, player_id, hit_player_id, 16, 1, 0, 5, 0, shot_pos);
                        break;
                    }
                    case HIT_TYPE_LEGS:
                    {
                        send_set_hp(server, player_id, hit_player_id, 16, 1, 0, 5, 0, shot_pos);
                        break;
                    }
                    case HIT_TYPE_MELEE:
                        // Empty so we dont have errors :)
                        break;
                }
                break;
            }
        }
        if (hit_type == HIT_TYPE_MELEE) {
            send_set_hp(server, player_id, hit_player_id, 80, 1, 2, 5, 0, shot_pos);
        }
    }
}

static void receive_orientation_data(server_t* server, uint8_t player_id, stream_t* data)
{
    float x, y, z;
    x                     = stream_read_f(data);
    y                     = stream_read_f(data);
    z                     = stream_read_f(data);
    float     length      = sqrt((x * x) + (y * y) + (z * z));
    float     norm_legnth = 1 / length;
    player_t* player      = &server->player[player_id];
    // Normalize the vectors if their length > 1
    if (length > 1.f) {
        player->movement.forward_orientation.x = x * norm_legnth;
        player->movement.forward_orientation.y = y * norm_legnth;
        player->movement.forward_orientation.z = z * norm_legnth;
    } else {
        player->movement.forward_orientation.x = x;
        player->movement.forward_orientation.y = y;
        player->movement.forward_orientation.z = z;
    }

    physics_reorient_player(server, player_id, &player->movement.forward_orientation);
}

static void receive_input_data(server_t* server, uint8_t player_id, stream_t* data)
{
    uint8_t   bits[8];
    uint8_t   mask        = 1;
    uint8_t   received_id = stream_read_u8(data);
    player_t* player      = &server->player[player_id];
    if (player_id != received_id) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in Input packet", player_id, received_id);
    } else if (player->state == STATE_READY) {
        player->input = stream_read_u8(data);
        for (int i = 0; i < 8; i++) {
            bits[i] = (player->input >> i) & mask;
        }
        player->move_forward   = bits[0];
        player->move_backwards = bits[1];
        player->move_left      = bits[2];
        player->move_right     = bits[3];
        player->jumping        = bits[4];
        player->crouching      = bits[5];
        player->sneaking       = bits[6];
        player->sprinting      = bits[7];
        send_input_data(server, player_id);
    }
}

static void receive_position_data(server_t* server, uint8_t player_id, stream_t* data)
{
    float x, y, z;
    x = stream_read_f(data);
    y = stream_read_f(data);
    z = stream_read_f(data);

    player_t* player = &server->player[player_id];
#ifdef DEBUG
    LOG_DEBUG("Player: %d, Our X: %f, Y: %f, Z: %f Actual X: %f, Y: %f, Z: %f",
              player_id,
              player->movement.position.x,
              player->movement.position.y,
              player->movement.position.z,
              x,
              y,
              z);
    LOG_DEBUG("Player: %d, Z solid: %d, Z+1 solid: %d, Z+2 solid: %d, Z: %d, Z+1: %d, Z+2: %d, Crouching: %d",
              player_id,
              mapvxl_is_solid(&server->s_map.map, (int) x, (int) y, (int) z),
              mapvxl_is_solid(&server->s_map.map, (int) x, (int) y, (int) z + 1),
              mapvxl_is_solid(&server->s_map.map, (int) x, (int) y, (int) z + 2),
              (int) z,
              (int) z + 1,
              (int) z + 2,
              player->crouching);
#endif
    player->movement.prev_position = player->movement.position;
    player->movement.position.x    = x;
    player->movement.position.y    = y;
    player->movement.position.z    = z;
    /*uint8_t resetTime                                = 1;
    if (!diff_is_older_then_dont_update(
        getNanos(), player->timers.duringNoclipPeriod, (uint64_t) NANO_IN_SECOND * 10))
    {
        resetTime = 0;
        if (validPlayerPos(server, player_id, x, y, z) == 0) {
            player->invalidPosCount++;
            if (player->invalidPosCount == 5) {
                sendMessageToStaff(server, "Player %s (#%hhu) is most likely trying to avoid noclip detection");
            }
        }
    }
    if (resetTime) {
        player->timers.duringNoclipPeriod = getNanos();
        player->invalidPosCount           = 0;
    }*/

    if (valid_pos_3f(server, player_id, x, y, z)) {
        player->movement.prev_legit_pos = player->movement.position;
    } /*else if (validPlayerPos(server,
                              player_id,
                              player->movement.position.x,
                              player->movement.position.y,
                              player->movement.position.z) == 0 &&
               validPlayerPos(server,
                              player_id,
                              player->movement.prevPosition.x,
                              player->movement.prevPosition.y,
                              player->movement.prevPosition.z) == 0)
    {
        LOG_WARNING("Player %s (#%hhu) may be noclipping!", player->name, player_id);
        sendMessageToStaff(server, "Player %s (#%d) may be noclipping!", player->name, player_id);
        if (validPlayerPos(server,
                           player_id,
                           player->movement.prevLegitPos.x,
                           player->movement.prevLegitPos.y,
                           player->movement.prevLegitPos.z) == 0)
        {
            if (getPlayerUnstuck(server, player_id) == 0) {
                SetPlayerRespawnPoint(server, player_id);
                sendServerNotice(
                server, player_id, 0, "Server could not find a free position to get you unstuck. Reseting to spawn");
                LOG_WARNING(
                "Could not find legit position for player %s (#%d) to get them unstuck. Resetting to spawn. "
                "Go check them!",
                player->name,
                player_id);
                sendMessageToStaff(
                server,
                "Could not find legit position for player %s (#%d) to get them unstuck. Resetting to spawn. "
                "Go check them!",
                player->name,
                player_id);
                player->movement.prevLegitPos = player->movement.position;
            }
        }
        SendPositionPacket(server,
                           player_id,
                           player->movement.prevLegitPos.x,
                           player->movement.prevLegitPos.y,
                           player->movement.prevLegitPos.z);
    }*/
}

static void receive_existing_player(server_t* server, uint8_t player_id, stream_t* data)
{
    stream_skip(data, 1); // Clients always send a "dumb" ID here since server has not sent them their ID yet

    player_t* player = &server->player[player_id];
    player->team     = stream_read_u8(data);
    player->weapon   = stream_read_u8(data);
    player->item     = stream_read_u8(data);
    player->kills    = stream_read_u32(data);

    if (player->team != 0 && player->team != 1 && player->team != 255) {
        LOG_WARNING("Player %s (#%hhu) sent invalid team. Switching them to Spectator", player->name, player_id);
        player->team = 255;
    }

    server->protocol.num_team_users[player->team]++;

    stream_read_color_rgb(data);
    player->ups = 60;

    uint32_t length  = stream_left(data);
    uint8_t  invName = 0;
    if (length > 16) {
        LOG_WARNING("Name of player %d is too long. Cutting", player_id);
        length = 16;
    } else {
        player->name[length] = '\0';
        stream_read_array(data, player->name, length);

        if (strlen(player->name) == 0) {
            snprintf(player->name, strlen("Deuce") + 1, "Deuce");
            length  = 5;
            invName = 1;
        } else if (player->name[0] == '#') {
            snprintf(player->name, strlen("Deuce") + 1, "Deuce");
            length  = 5;
            invName = 1;
        }

        char* lowerCaseName = malloc((strlen(player->name) + 1) * sizeof(char));
        snprintf(lowerCaseName, strlen(player->name), "%s", player->name);

        for (uint32_t i = 0; i < strlen(player->name); ++i)
            lowerCaseName[i] = tolower(lowerCaseName[i]);

        char* unwantedNames[] = {"igger", "1gger", "igg3r", "1gg3r", NULL};

        int index = 0;

        while (unwantedNames[index] != NULL) {
            if (strstr(lowerCaseName, unwantedNames[index]) != NULL &&
                strcmp(unwantedNames[index], strstr(lowerCaseName, unwantedNames[index])) == 0)
            {
                snprintf(player->name, strlen("Deuce") + 1, "Deuce");
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
            if (is_past_join_screen(server, i) && i != player_id) {
                if (strcmp(player->name, server->player[i].name) == 0) {
                    count++;
                }
            }
        }
        if (count > 0) {
            char idChar[4];
            snprintf(idChar, 4, "%d", player_id);
            strlcat(player->name, idChar, 17);
        }
    }
    switch (player->weapon) {
        case 0:
            player->weapon_reserve  = 50;
            player->weapon_clip     = 10;
            player->default_clip    = RIFLE_DEFAULT_CLIP;
            player->default_reserve = RIFLE_DEFAULT_RESERVE;
            break;
        case 1:
            player->weapon_reserve  = 120;
            player->weapon_clip     = 30;
            player->default_clip    = SMG_DEFAULT_CLIP;
            player->default_reserve = SMG_DEFAULT_RESERVE;
            break;
        case 2:
            player->weapon_reserve  = 48;
            player->weapon_clip     = 6;
            player->default_clip    = SHOTGUN_DEFAULT_CLIP;
            player->default_reserve = SHOTGUN_DEFAULT_RESERVE;
            break;
    }
    player->state = STATE_SPAWNING;
    char IP[17];
    format_ip_to_str(IP, player->ip);
    char team[15];
    team_id_to_str(server, team, player->team);
    LOG_INFO("Player %s (%s, #%hhu) joined %s", player->name, IP, player_id, team);
    if (player->welcome_sent == 0) {
        string_node_t* welcomeMessage;
        DL_FOREACH(server->welcome_messages, welcomeMessage)
        {
            send_server_notice(server, player_id, 0, welcomeMessage->string);
        }
        if (invName) {
            send_server_notice(
            server,
            player_id,
            0,
            "Your name was either empty, had # in front of it or contained something nasty. Your name "
            "has been set to %s",
            player->name);
        }
        player->welcome_sent = 1; // So we dont send the message to the player on each time they spawn.
    }

    if (server->protocol.gamemode.intel_held[0] == 0) {
        send_move_object(server, 0, 0, server->protocol.gamemode.intel[0]);
    }
    if (server->protocol.gamemode.intel_held[1] == 0) {
        send_move_object(server, 1, 1, server->protocol.gamemode.intel[1]);
    }
}

static void receive_block_action(server_t* server, uint8_t player_id, stream_t* data)
{
    uint8_t received_id = stream_read_u8(data);
    if (player_id != received_id) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in block action packet", player_id, received_id);
    }
    player_t* player = &server->player[player_id];
    if (player->can_build && server->global_ab) {
        uint8_t  action_type = stream_read_u8(data);
        uint32_t X           = stream_read_u32(data);
        uint32_t Y           = stream_read_u32(data);
        uint32_t Z           = stream_read_u32(data);
        if (player->sprinting) {
            return;
        }
        vector3i_t vectorBlock  = {X, Y, Z};
        vector3f_t vectorfBlock = {(float) X, (float) Y, (float) Z};
        vector3f_t playerVector = player->movement.position;
        if (((player->item == 0 && (action_type == 1 || action_type == 2)) || (player->item == 1 && action_type == 0) ||
             (player->item == 2 && action_type == 1)))
        {
            if ((distance_in_3d(vectorfBlock, playerVector) <= 4 || player->item == 2) &&
                valid_pos_v3i(server, vectorBlock)) {
                switch (action_type) {
                    case 0:
                    {
                        if (gamemode_block_checks(server, X, Y, Z)) {
                            uint64_t timeNow = get_nanos();
                            if (player->blocks > 0 &&
                                diff_is_older_then(timeNow, &player->timers.since_last_block_plac, BLOCK_DELAY) &&
                                diff_is_older_then_dont_update(
                                timeNow, player->timers.since_last_block_dest, BLOCK_DELAY) &&
                                diff_is_older_then_dont_update(
                                timeNow, player->timers.since_last_3block_dest, BLOCK_DELAY))
                            {
                                mapvxl_set_color(&server->s_map.map, X, Y, Z, player->tool_color.raw);
                                player->blocks--;
                                moveIntelAndTentUp(server);
                                send_block_action(server, player_id, action_type, X, Y, Z);
                            }
                        }
                    } break;

                    case 1:
                    {
                        if (Z < 62 && gamemode_block_checks(server, X, Y, Z)) {
                            uint64_t timeNow = get_nanos();
                            if ((diff_is_older_then(timeNow, &player->timers.since_last_block_dest, SPADE_DELAY) &&
                                 diff_is_older_then_dont_update(
                                 timeNow, player->timers.since_last_3block_dest, SPADE_DELAY) &&
                                 diff_is_older_then_dont_update(
                                 timeNow, player->timers.since_last_block_plac, SPADE_DELAY)) ||
                                player->item == 2)
                            {
                                if (player->item == 2) {
                                    if (player->weapon_clip <= 0) {
                                        send_message_to_staff(server,
                                                              "Player %s (#%hhu) probably has hack to have more ammo",
                                                              player->name,
                                                              player_id);
                                        return;
                                    } else if (player->weapon == 0 &&
                                               diff_is_older_then(timeNow,
                                                                  &player->timers.since_last_block_dest_with_gun,
                                                                  ((RIFLE_DELAY * 2) - (NANO_IN_MILLI * 10))) == 0)
                                    {
                                        send_message_to_staff(server,
                                                              "Player %s (#%hhu) probably has rapid shooting hack",
                                                              player->name,
                                                              player_id);
                                        return;
                                    } else if (player->weapon == 1 &&
                                               diff_is_older_then(timeNow,
                                                                  &player->timers.since_last_block_dest_with_gun,
                                                                  ((SMG_DELAY * 3) - (NANO_IN_MILLI * 10))) == 0)
                                    {
                                        send_message_to_staff(server,
                                                              "Player %s (#%hhu) probably has rapid shooting hack",
                                                              player->name,
                                                              player_id);
                                        return;
                                    } else if (player->weapon == 2 &&
                                               diff_is_older_then(timeNow,
                                                                  &player->timers.since_last_block_dest_with_gun,
                                                                  (SHOTGUN_DELAY - (NANO_IN_MILLI * 10))) == 0)
                                    {
                                        send_message_to_staff(server,
                                                              "Player %s (#%hhu) probably has rapid shooting hack",
                                                              player->name,
                                                              player_id);
                                        return;
                                    }
                                }

                                vector3i_t  position = {X, Y, Z};
                                vector3i_t* neigh    = get_neighbors(position);
                                mapvxl_set_air(&server->s_map.map, position.x, position.y, position.z);
                                for (int i = 0; i < 6; ++i) {
                                    if (neigh[i].z < 62) {
                                        check_node(server, neigh[i]);
                                    }
                                }
                                if (player->item != 2) {
                                    if (player->blocks < 50) {
                                        player->blocks++;
                                    }
                                }
                                send_block_action(server, player_id, action_type, X, Y, Z);
                            }
                        }
                    } break;

                    case 2:
                    {
                        if (gamemode_block_checks(server, X, Y, Z) && gamemode_block_checks(server, X, Y, Z + 1) &&
                            gamemode_block_checks(server, X, Y, Z - 1))
                        {
                            uint64_t timeNow = get_nanos();
                            if (diff_is_older_then(timeNow, &player->timers.since_last_3block_dest, THREEBLOCK_DELAY) &&
                                diff_is_older_then_dont_update(
                                timeNow, player->timers.since_last_block_dest, THREEBLOCK_DELAY) &&
                                diff_is_older_then_dont_update(
                                timeNow, player->timers.since_last_block_plac, THREEBLOCK_DELAY))
                            {
                                for (uint32_t z = Z - 1; z <= Z + 1; z++) {
                                    if (z < 62) {
                                        mapvxl_set_air(&server->s_map.map, X, Y, z);
                                        vector3i_t  position = {X, Y, z};
                                        vector3i_t* neigh    = get_neighbors(position);
                                        mapvxl_set_air(&server->s_map.map, position.x, position.y, position.z);
                                        for (int i = 0; i < 6; ++i) {
                                            if (neigh[i].z < 62) {
                                                check_node(server, neigh[i]);
                                            }
                                        }
                                    }
                                }
                                send_block_action(server, player_id, action_type, X, Y, Z);
                            }
                        }
                    } break;
                }
                move_intel_and_tent_down(server);
            }
        } else {
            LOG_WARNING("Player %s (#%hhu) may be using BlockExploit with Item: %d and Action: %d",
                        player->name,
                        player_id,
                        player->item,
                        action_type);
        }
    }
}

static void receive_block_line(server_t* server, uint8_t player_id, stream_t* data)
{
    uint8_t received_id = stream_read_u8(data);
    if (player_id != received_id) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in blockline packet", player_id, received_id);
    }
    uint64_t  time_now = get_nanos();
    player_t* player   = &server->player[player_id];
    if (player->blocks > 0 && player->can_build && server->global_ab && player->item == 1 &&
        diff_is_older_then(time_now, &player->timers.since_last_block_plac, BLOCK_DELAY) &&
        diff_is_older_then_dont_update(time_now, player->timers.since_last_block_dest, BLOCK_DELAY) &&
        diff_is_older_then_dont_update(time_now, player->timers.since_last_3block_dest, BLOCK_DELAY))
    {
        vector3i_t start;
        vector3i_t end;
        start.x = stream_read_u32(data);
        start.y = stream_read_u32(data);
        start.z = stream_read_u32(data);
        end.x   = stream_read_u32(data);
        end.y   = stream_read_u32(data);
        end.z   = stream_read_u32(data);

        player_t* player = &server->player[player_id];
        if (player->sprinting) {
            return;
        }
        vector3f_t startF = {start.x, start.y, start.z};
        vector3f_t endF   = {end.x, end.y, end.z};
        if (distance_in_3d(endF, player->movement.position) <= 4 && distance_in_3d(startF, player->locAtClick) <= 4 &&
            valid_pos_v3f(server, startF) && valid_pos_v3f(server, endF))
        {
            int size = line_get_blocks(&start, &end, server->s_map.result_line);
            player->blocks -= size;
            for (int i = 0; i < size; i++) {
                mapvxl_set_color(&server->s_map.map,
                                 server->s_map.result_line[i].x,
                                 server->s_map.result_line[i].y,
                                 server->s_map.result_line[i].z,
                                 player->tool_color.raw);
            }
            moveIntelAndTentUp(server);
            send_block_line(server, player_id, start, end);
        }
    }
}

static void receive_set_tool(server_t* server, uint8_t player_id, stream_t* data)
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

static void receive_set_color(server_t* server, uint8_t player_id, stream_t* data)
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

static void receive_weapon_input(server_t* server, uint8_t player_id, stream_t* data)
{
    uint8_t   mask           = 1;
    uint8_t   received_id    = stream_read_u8(data);
    uint8_t   received_input = stream_read_u8(data);
    player_t* player         = &server->player[player_id];
    if (player_id != received_id) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in weapon input packet", player_id, received_id);
    } else if (player->state != STATE_READY) {
        return;
    } else if (player->sprinting) {
        received_input = 0; /* Do not return just set it to 0 as we want to send to players that the player is no longer
                    shooting when they start sprinting */
    }
    player->primary_fire   = received_input & mask;
    player->secondary_fire = (received_input >> 1) & mask;

    if (player->secondary_fire && player->item == 1) {
        player->locAtClick = player->movement.position;
    } else if (player->secondary_fire && player->item == 0) {
        player->timers.since_possible_spade_nade = get_nanos();
    }

    else if (player->weapon_clip > 0)
    {
        send_weapon_input(server, player_id, received_input);
        uint64_t timeDiff = 0;
        switch (player->weapon) {
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

        if (player->primary_fire && diff_is_older_then(get_nanos(), &player->timers.since_last_weapon_input, timeDiff))
        {
            player->timers.since_last_weapon_input = get_nanos();
            player->weapon_clip--;
            player->reloading = 0;
            if ((player->movement.previous_orientation.x == player->movement.forward_orientation.x) &&
                (player->movement.previous_orientation.y == player->movement.forward_orientation.y) &&
                (player->movement.previous_orientation.z == player->movement.forward_orientation.z) &&
                player->item == TOOL_GUN)
            {
                for (int i = 0; i < server->protocol.max_players; ++i) {
                    if (is_past_join_screen(server, i) && isStaff(server, i)) {
                        char message[200];
                        snprintf(message, 200, "WARNING. Player %d may be using no recoil", player_id);
                        send_server_notice(server, i, 0, message);
                    }
                }
            }
            player->movement.previous_orientation = player->movement.forward_orientation;
        }
    } else {
        // sendKillActionPacket(server, player_id, player_id, 0, 30, 0);
    }
}

static void receive_weapon_reload(server_t* server, uint8_t player_id, stream_t* data)
{
    uint8_t ID      = stream_read_u8(data);
    uint8_t clip    = stream_read_u8(data);
    uint8_t reserve = stream_read_u8(data);
    if (player_id != ID) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in weapon reload packet", player_id, ID);
    }
    player_t* player       = &server->player[player_id];
    player->primary_fire   = 0;
    player->secondary_fire = 0;

    if (player->weapon_reserve == 0 || player->reloading) {
        return;
    }
    player->reloading                 = 1;
    player->timers.since_reload_start = get_nanos();
    send_weapon_reload(server, player_id, 1, clip, reserve);
}

static void receive_change_team(server_t* server, uint8_t player_id, stream_t* data)
{
    uint8_t   received_id = stream_read_u8(data);
    player_t* player      = &server->player[player_id];
    server->protocol.num_team_users[player->team]--;
    uint8_t team = stream_read_u8(data);

    if (team != 0 && team != 1 && team != 255) {
        LOG_WARNING("Player %s (#%hhu) sent invalid team. Switching them to Spectator", player->name, player_id);
        team = 255;
    }

    player->team = team;
    if (player_id != received_id) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in change team packet", player_id, received_id);
    }
    server->protocol.num_team_users[player->team]++;
    send_kill_action_packet(server, player_id, player_id, 5, 5, 0);
    player->state = STATE_WAITING_FOR_RESPAWN;
}

static void receive_change_weapon(server_t* server, uint8_t player_id, stream_t* data)
{
    uint8_t   received_id     = stream_read_u8(data);
    uint8_t   received_weapon = stream_read_u8(data);
    player_t* player          = &server->player[player_id];
    if (player->weapon == received_weapon) {
        return;
    }
    player->weapon = received_weapon;
    if (player_id != received_id) {
        LOG_WARNING("Assigned ID: %d doesnt match sent ID: %d in change weapon packet", player_id, received_id);
    }
    send_kill_action_packet(server, player_id, player_id, 6, 5, 0);
    player->state = STATE_WAITING_FOR_RESPAWN;
}

static void receive_version_response(server_t* server, uint8_t player_id, stream_t* data)
{
    player_t* player         = &server->player[player_id];
    player->client           = stream_read_u8(data);
    player->version_major    = stream_read_u8(data);
    player->version_minor    = stream_read_u8(data);
    player->version_revision = stream_read_u8(data);
    uint32_t length          = stream_left(data);
    if (length < 256) {
        player->os_info[length] = '\0';
        stream_read_array(data, player->os_info, length);
    } else {
        snprintf(player->os_info, 8, "Unknown");
    }
    if (player->client == 'o') {
        if (!(player->version_major == 0 && player->version_minor == 1 &&
              (player->version_revision == 3 || player->version_revision == 5)))
        {
            enet_peer_disconnect(player->peer, REASON_KICKED);
        }
    } else if (player->client == 'B') {
        if (!(player->version_major == 0 && player->version_minor == 1 && player->version_revision == 5)) {
            enet_peer_disconnect(player->peer, REASON_KICKED);
        }
    }
}

void init_packets(server_t* server)
{
    server->packets            = NULL;
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
        packet_t* packet = malloc(sizeof(packet_t));
        packet->id       = packets[i].id;
        packet->packet   = packets[i].packet;
        HASH_ADD_INT(server->packets, id, packet);
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

void on_packet_received(server_t* server, uint8_t playerID, stream_t* data)
{
    uint8_t   type     = stream_read_u8(data);
    packet_t* packet_p = NULL;
    int       type_int = (int) type;
    HASH_FIND_INT(server->packets, &type_int, packet_p);
    if (packet_p == NULL) {
        LOG_WARNING("Unknown packet with ID %d received", type_int);
        return;
    }
    packet_p->packet(server, playerID, data);
}
