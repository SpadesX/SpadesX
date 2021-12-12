// Copyright DarkNeutrino 2021
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "Enums.h"
#include "Map.h"

#include <DataStream.h>
#include <Queue.h>
#include <Types.h>
#include <enet/enet.h>

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
void      sendServerNotice(Server* server, uint8 playerID, char* message);
void      broadcastServerNotice(Server* server, char* message);
uint8     SendPacketExceptSender(Server* server, ENetPacket* packet, uint8 playerID);
uint8     SendPacketExceptSenderDistCheck(Server* server, ENetPacket* packet, uint8 playerID);
uint8     SendPacketDistCheck(Server* server, ENetPacket* packet, uint8 playerID);
uint8     playerToPlayerVisible(Server* server, uint8 playerID, uint8 playerID2);
uint32    DistanceIn3D(Vector3f vector1, Vector3f vector2);
uint32    DistanceIn2D(Vector3f vector1, Vector3f vector2);
uint8     Collision3D(Vector3f vector1, Vector3f vector2, uint8 distance);
uint8     diffIsOlderThen(time_t timeNow, time_t* timeBefore, time_t timeDiff);
uint8     diffIsOlderThenDontUpdate(time_t timeNow, time_t timeBefore, time_t timeDiff);
uint8     isStaff(Server* server, uint8 playerID);
void      sendMessageToStaff(Server* server, char* message);

#endif
