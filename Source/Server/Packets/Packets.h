// Copyright DarkNeutrino 2021
#ifndef PACKETS_H
#define PACKETS_H

#include <Server/Structs/ServerStruct.h>
#include <Util/DataStream.h>
#include <Util/Line.h>
#include <Util/Queue.h>
#include <Util/Types.h>

uint8_t allow_shot(server_t*  server,
                   player_t*  player,
                   player_t*  player_hit,
                   uint64_t   time_now,
                   float      distance,
                   long*      x,
                   long*      y,
                   long*      z,
                   vector3f_t shot_pos,
                   vector3f_t shot_orien,
                   vector3f_t hit_pos,
                   vector3f_t shot_eye_pos); // Remove me from here later

void send_restock(server_t* server, player_t* player);
void send_move_object(server_t* server, uint8_t object, uint8_t team, vector3f_t pos);
void send_intel_capture(server_t* server, player_t* player, uint8_t winning);
void send_intel_pickup(server_t* server, player_t* player);
void send_intel_drop(server_t* server, player_t* player);
void send_grenade(server_t* server, player_t* player, float fuse, vector3f_t position, vector3f_t velocity);
void send_player_left(server_t* server, player_t* player);
void send_weapon_reload(server_t* server, player_t* player, uint8_t startAnimation, uint8_t clip, uint8_t reserve);
void send_weapon_input(server_t* server, player_t* player, uint8_t wInput);
void send_set_color(server_t* server, player_t* player, color_t color);
void send_set_color_to_player(server_t* server, player_t* player, player_t* receiver, color_t color);
void send_set_tool(server_t* server, player_t* player, uint8_t tool);
void send_block_line(server_t* server, player_t* player, vector3i_t start, vector3i_t end);
void send_block_line_to_player(server_t*  server,
                               player_t*  player,
                               player_t*  receiver,
                               vector3i_t start,
                               vector3i_t end);
void send_block_action(server_t* server, player_t* player, uint8_t actionType, int X, int Y, int Z);

void send_block_action_to_player(server_t* server,
                                 player_t* player,
                                 player_t* receiver,
                                 uint8_t   actionType,
                                 int       X,
                                 int       Y,
                                 int       Z);

void send_state_data(server_t* server, player_t* player);
void send_input_data(server_t* server, player_t* player);
void send_kill_action_packet(server_t* server,
                             player_t* killer,
                             player_t* player,
                             uint8_t   killReason,
                             uint8_t   respawnTime,
                             uint8_t   makeInvisible);
void send_set_hp(server_t*  server,
                 player_t*  player,
                 player_t*  player_hit,
                 long       hp_chnage,
                 uint8_t    type_of_damage,
                 uint8_t    kill_reason,
                 uint8_t    respawn_time,
                 uint8_t    is_grenade,
                 vector3f_t position);
void send_existing_player(server_t* server, player_t* receiver, player_t* existing_player);
void send_map_start(server_t* server, player_t* player);
void send_map_chunks(server_t* server, player_t* player);
void send_create_player(server_t* server, player_t* receiver, player_t* child);
void send_respawn(server_t* server, player_t* respawn_player);
void send_world_update(server_t* server, player_t* player);
void send_position_packet(server_t* server, player_t* player, float x, float y, float z);
void send_version_request(server_t* server, player_t* player);

void init_packets(server_t* server);
void free_all_packets(server_t* server);

void on_packet_received(server_t* server, player_t* player, stream_t* data);

#endif
