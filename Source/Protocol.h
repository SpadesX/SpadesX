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
void     init_player(server_t* server,
        uint8_t                player_id,
        uint8_t                reset,
        uint8_t                disconnect,
        vector3f_t             empty,
        vector3f_t             forward,
        vector3f_t             strafe,
        vector3f_t             height);

uint8_t     octets_in_range(ip_t start, ip_t end, ip_t host);
uint8_t     ip_in_range(ip_t host, ip_t banned, ip_t startOfRange, ip_t endOfRange);
uint8_t     player_has_permission(server_t* server, uint8_t playerID, uint8_t console, uint32_t permission);
uint8_t     gamemode_block_checks(server_t* server, int x, int y, int z);
uint8_t     is_past_state_data(server_t* server, uint8_t playerID);
uint8_t     is_past_join_screen(server_t* server, uint8_t playerID);
uint8_t     valid_pos_v3i(server_t* server, vector3i_t pos);
uint8_t     valid_pos_v3f(server_t* server, vector3f_t pos);
uint8_t     valid_pos_3i(server_t* server, int x, int y, int z);
uint8_t     valid_pos_3f(server_t* server, uint8_t playerID, float X, float Y, float Z);
vector3i_t* get_neighbours(vector3i_t pos);
uint8_t     check_node(server_t* server, vector3i_t position);
void        moveIntelAndTentDown(server_t* server);
void        moveIntelAndTentUp(server_t* server);
uint8_t     checkUnderTent(server_t* server, uint8_t team);
uint8_t     checkUnderIntel(server_t* server, uint8_t team);
uint8_t     checkPlayerOnIntel(server_t* server, uint8_t playerID, uint8_t team);
uint8_t     checkPlayerInTent(server_t* server, uint8_t playerID);
uint8_t     checkItemOnIntel(server_t* server, uint8_t team, vector3f_t itemPos);
uint8_t     checkItemInTent(server_t* server, uint8_t team, vector3f_t itemPos);
uint8_t     checkInTent(server_t* server, uint8_t team);
uint8_t     checkInIntel(server_t* server, uint8_t team);
void        handleGrenade(server_t* server, uint8_t playerID);
void        updateMovementAndGrenades(server_t* server);
void        SetPlayerRespawnPoint(server_t* server, uint8_t playerID);
vector3f_t  SetIntelTentSpawnPoint(server_t* server, uint8_t team);
void        send_server_notice(server_t* server, uint8_t playerID, uint8_t console, const char* message, ...);
void        broadcast_server_notice(server_t* server, uint8_t console, const char* message, ...);
uint8_t     SendPacketExceptSender(server_t* server, ENetPacket* packet, uint8_t playerID);
uint8_t     SendPacketExceptSenderDistCheck(server_t* server, ENetPacket* packet, uint8_t playerID);
uint8_t     SendPacketDistCheck(server_t* server, ENetPacket* packet, uint8_t playerID);
uint8_t     playerToPlayerVisible(server_t* server, uint8_t playerID, uint8_t playerID2);
uint32_t    DistanceIn3D(vector3f_t vector1, vector3f_t vector2);
uint32_t    DistanceIn2D(vector3f_t vector1, vector3f_t vector2);
uint8_t     Collision3D(vector3f_t vector1, vector3f_t vector2, uint8_t distance);
uint8_t     diffIsOlderThen(uint64_t timeNow, uint64_t* timeBefore, uint64_t timeDiff);
uint8_t     diffIsOlderThenDontUpdate(uint64_t timeNow, uint64_t timeBefore, uint64_t timeDiff);
uint8_t     isStaff(server_t* server, uint8_t playerID);
void        sendMessageToStaff(server_t* server, const char* format, ...);
uint8_t     getGrenadeDamage(server_t* server, uint8_t damageID, grenade_t* grenade);
uint8_t     getPlayerUnstuck(server_t* server, uint8_t playerID);

#endif
