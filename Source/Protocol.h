// Copyright DarkNeutrino 2021
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "Map.h"

#include <DataStream.h>
#include <Enums.h>
#include <Queue.h>
#include <Types.h>
#include <enet/enet.h>
#include <stdarg.h>

void      initPlayer(Server*  server,
                     uint8    playerID,
                     uint8    reset,
                     uint8    disconnect,
                     Vector3f empty,
                     Vector3f forward,
                     Vector3f strafe,
                     Vector3f height);
uint8     playerHasPermission(Server* server, uint8 playerID, uint32 permission);
uint8     gamemodeBlockChecks(Server* server, int x, int y, int z);
uint8     isPastStateData(Server* server, uint8 playerID);
uint8     isPastJoinScreen(Server* server, uint8 playerID);
uint8     vecValidPos(Vector3i pos);
uint8     vecfValidPos(Vector3f pos);
uint8     validPos(int x, int y, int z);
uint8     validPlayerPos(Server* server, uint8 playerID, float X, float Y, float Z);
Vector3i* getNeighbors(Vector3i pos);
uint8     checkNode(Server* server, Vector3i position);
void      moveIntelAndTentDown(Server* server);
void      moveIntelAndTentUp(Server* server);
uint8     checkUnderTent(Server* server, uint8 team);
uint8     checkUnderIntel(Server* server, uint8 team);
uint8     checkPlayerOnIntel(Server* server, uint8 playerID, uint8 team);
uint8     checkPlayerInTent(Server* server, uint8 playerID);
uint8     checkItemOnIntel(Server* server, uint8 team, Vector3f itemPos);
uint8     checkItemInTent(Server* server, uint8 team, Vector3f itemPos);
uint8     checkInTent(Server* server, uint8 team);
uint8     checkInIntel(Server* server, uint8 team);
void      handleGrenade(Server* server, uint8 playerID);
void      updateMovementAndGrenades(Server* server);
void      SetPlayerRespawnPoint(Server* server, uint8 playerID);
Vector3f  SetIntelTentSpawnPoint(Server* server, uint8 team);
void      sendServerNotice(Server* server, uint8 playerID, const char* message, ...);
void broadcastServerNotice(Server* server, const char* message, ...);
uint8     SendPacketExceptSender(Server* server, ENetPacket* packet, uint8 playerID);
uint8     SendPacketExceptSenderDistCheck(Server* server, ENetPacket* packet, uint8 playerID);
uint8     SendPacketDistCheck(Server* server, ENetPacket* packet, uint8 playerID);
uint8     playerToPlayerVisible(Server* server, uint8 playerID, uint8 playerID2);
uint32    DistanceIn3D(Vector3f vector1, Vector3f vector2);
uint32    DistanceIn2D(Vector3f vector1, Vector3f vector2);
uint8     Collision3D(Vector3f vector1, Vector3f vector2, uint8 distance);
uint8     diffIsOlderThen(uint64 timeNow, uint64* timeBefore, uint64 timeDiff);
uint8     diffIsOlderThenDontUpdate(uint64 timeNow, uint64 timeBefore, uint64 timeDiff);
uint8     isStaff(Server* server, uint8 playerID);
void      sendMessageToStaff(Server* server, const char* format, ...);
uint8 getGrenadeDamage(Server* server, uint8 damageID, Grenade* grenade);
uint8     getPlayerUnstuck(Server* server, uint8 playerID);

#endif
