// Copyright DarkNeutrino 2021
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "Map.h"
#include "Util/DataStream.h"
#include "Util/Enums.h"
#include "Util/Queue.h"
#include "Util/Types.h"

#include <enet/enet.h>
#include <stdarg.h>

uint64_t get_nanos(void);
void     init_player(server_t*  server,
                     uint8_t    player_id,
                     uint8_t    reset,
                     uint8_t    disconnect,
                     vector3f_t empty,
                     vector3f_t forward,
                     vector3f_t strafe,
                     vector3f_t height);

uint8_t     octets_in_range(ip_t start, ip_t end, ip_t host);
uint8_t     ip_in_range(ip_t host, ip_t banned, ip_t startOfRange, ip_t endOfRange);
uint8_t     player_has_permission(server_t* server, uint8_t player_id, uint8_t console, uint32_t permission);
uint8_t     gamemode_block_checks(server_t* server, int x, int y, int z);
uint8_t     is_past_state_data(server_t* server, uint8_t player_id);
uint8_t     is_past_join_screen(server_t* server, uint8_t player_id);
uint8_t     valid_pos_v3i(server_t* server, vector3i_t pos);
uint8_t     valid_pos_v3f(server_t* server, vector3f_t pos);
uint8_t     valid_pos_3i(server_t* server, int x, int y, int z);
uint8_t     valid_pos_3f(server_t* server, uint8_t player_id, float X, float Y, float Z);
vector3i_t* get_neighbours(vector3i_t pos);
uint8_t     check_node(server_t* server, vector3i_t position);
void        move_intel_and_tent_down(server_t* server);
void        moveIntelAndTentUp(server_t* server);
uint8_t     check_under_tent(server_t* server, uint8_t team);
uint8_t     check_under_intel(server_t* server, uint8_t team);
uint8_t     check_player_on_intel(server_t* server, uint8_t player_id, uint8_t team);
uint8_t     check_player_in_tent(server_t* server, uint8_t player_id);
uint8_t     check_item_on_intel(server_t* server, uint8_t team, vector3f_t itemPos);
uint8_t     check_item_in_tent(server_t* server, uint8_t team, vector3f_t itemPos);
uint8_t     check_in_tent(server_t* server, uint8_t team);
uint8_t     check_in_intel(server_t* server, uint8_t team);
void        handle_grenade(server_t* server, uint8_t player_id);
void        update_movement_and_grenades(server_t* server);
void        set_player_respawn_point(server_t* server, uint8_t player_id);
vector3f_t  set_intel_tent_spawn_point(server_t* server, uint8_t team);
void        send_server_notice(server_t* server, uint8_t player_id, uint8_t console, const char* message, ...);
void        broadcast_server_notice(server_t* server, uint8_t console, const char* message, ...);
uint8_t     send_packet_except_sender(server_t* server, ENetPacket* packet, uint8_t player_id);
uint8_t     send_packet_except_sender_dist_check(server_t* server, ENetPacket* packet, uint8_t player_id);
uint8_t     send_packet_dist_check(server_t* server, ENetPacket* packet, uint8_t player_id);
uint8_t     player_to_player_visibile(server_t* server, uint8_t player_id, uint8_t player_id2);
uint32_t    distance_in_3d(vector3f_t vector1, vector3f_t vector2);
uint32_t    distance_in_2d(vector3f_t vector1, vector3f_t vector2);
uint8_t     collision_3d(vector3f_t vector1, vector3f_t vector2, uint8_t distance);
uint8_t     diff_is_older_then(uint64_t timeNow, uint64_t* timeBefore, uint64_t timeDiff);
uint8_t     diff_is_older_then_dont_update(uint64_t timeNow, uint64_t timeBefore, uint64_t timeDiff);
uint8_t     is_staff(server_t* server, uint8_t player_id);
void        send_message_to_staff(server_t* server, const char* format, ...);
uint8_t     get_grenade_damage(server_t* server, uint8_t damageID, grenade_t* grenade);
uint8_t     get_player_unstuck(server_t* server, uint8_t player_id);

#endif
