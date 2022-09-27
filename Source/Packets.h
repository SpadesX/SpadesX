// Copyright DarkNeutrino 2021
#ifndef PACKETS_H
#define PACKETS_H

#include "Structs.h"
#include "Util/DataStream.h"
#include "Util/Line.h"
#include "Util/Queue.h"
#include "Util/Types.h"

void send_restock(server_t* server, uint8_t playerID);
void send_move_object(server_t* server, uint8_t object, uint8_t team, vector3f_t pos);
void send_intel_capture(server_t* server, uint8_t playerID, uint8_t winning);
void send_intel_pickup(server_t* server, uint8_t playerID);
void send_intel_drop(server_t* server, uint8_t playerID);
void send_grenade(server_t* server, uint8_t playerID, float fuse, vector3f_t position, vector3f_t velocity);
void send_player_left(server_t* server, uint8_t playerID);
void send_weapon_reload(server_t* server, uint8_t playerID, uint8_t startAnimation, uint8_t clip, uint8_t reserve);
void send_weapon_input(server_t* server, uint8_t playerID, uint8_t wInput);
void send_set_color(server_t* server, uint8_t playerID, uint8_t R, uint8_t G, uint8_t B);
void send_set_color_to_player(server_t* server, uint8_t playerID, uint8_t sendToID, uint8_t R, uint8_t G, uint8_t B);
void send_set_tool(server_t* server, uint8_t playerID, uint8_t tool);
void send_block_line(server_t* server, uint8_t playerID, vector3i_t start, vector3i_t end);
void block_line_to_player(server_t* server, uint8_t playerID, uint8_t sendToID, vector3i_t start, vector3i_t end);
void send_block_action(server_t* server, uint8_t playerID, uint8_t actionType, int X, int Y, int Z);

void send_block_action_to_player(server_t* server,
    uint8_t                                playerID,
    uint8_t                                sendToID,
    uint8_t                                actionType,
    int                                    X,
    int                                    Y,
    int                                    Z);

void send_state_data(server_t* server, uint8_t playerID);
void send_input_data(server_t* server, uint8_t playerID);
void send_kill_action_packet(server_t* server,
    uint8_t                            killerID,
    uint8_t                            playerID,
    uint8_t                            killReason,
    uint8_t                            respawnTime,
    uint8_t                            makeInvisible);
void send_set_hp(server_t* server,
    uint8_t                playerID,
    uint8_t                hitPlayerID,
    long                   HPChange,
    uint8_t                typeOfDamage,
    uint8_t                killReason,
    uint8_t                respawnTime,
    uint8_t                isGrenade,
    vector3f_t             position);
void send_existing_player(server_t* server, uint8_t playerID, uint8_t otherID);
void send_map_start(server_t* server, uint8_t playerID);
void send_map_chunks(server_t* server, uint8_t playerID);
void send_create_player(server_t* server, uint8_t playerID, uint8_t otherID);
void send_respawn(server_t* server, uint8_t playerID);
void handle_and_send_message(ENetEvent event, datastream_t* data, server_t* server, uint8_t playerID);
void send_world_update(server_t* server, uint8_t playerID);
void send_position_packet(server_t* server, uint8_t playerID, float x, float y, float z);

void on_packet_received(server_t* server, uint8_t playerID, datastream_t* data, ENetEvent event);

#endif
