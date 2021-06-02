//Copyright DarkNeutrino 2021
#ifndef PACKETS_H
#define PACKETS_H

#include "Line.h"
#include "Structs.h"
#include "Queue.h"
#include "Types.h"
#include "DataStream.h"

void SendIntelCapture(Server* server, uint8 playerID, uint8 winning);
void SendIntelPickup(Server* server, uint8 playerID);
void SendIntelDrop(Server* server, uint8 playerID);
void SendGrenade(Server* server, uint8 playerID, float fuse, Vector3f position, Vector3f velocity);
void SendPlayerLeft(Server* server, uint8 playerID);
void SendWeaponReload(Server* server, uint8 playerID);
void SendWeaponInput(Server* server, uint8 playerID, uint8 wInput);
void SendSetColor(Server* server, uint8 playerID, uint8 R, uint8 G, uint8 B);
void SendSetTool(Server* server, uint8 playerID, uint8 tool);
void SendBlockLine(Server* server, uint8 playerID, vec3i start, vec3i end);
void SendBlockAction(Server* server, uint8 playerID, uint8 actionType, int X, int Y, int Z);
void SendStateData(Server* server, uint8 playerID);
void SendInputData(Server* server, uint8 playerID);
void sendKillPacket(Server* server, uint8 killerID, uint8 playerID, uint8 killReason, uint8 respawnTime);
void sendHP(Server* server, uint8 hitPlayerID, uint8 playerID, long HPChange, uint8 type, uint8 killReason, uint8 respawnTime);
void SendPlayerState(Server* server, uint8 playerID, uint8 otherID);
void SendMapStart(Server* server, uint8 playerID);
void SendMapChunks(Server* server, uint8 playerID);
void SendRespawnState(Server* server, uint8 playerID, uint8 otherID);
void SendRespawn(Server* server, uint8 playerID);
void sendMessage(ENetEvent event, DataStream* data, Server* server);
void SendWorldUpdate(Server* server, uint8 playerID);

#endif
