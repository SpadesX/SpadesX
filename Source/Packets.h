// Copyright DarkNeutrino 2021
#ifndef PACKETS_H
#define PACKETS_H

#include "Structs.h"

#include <DataStream.h>
#include <Line.h>
#include <Queue.h>
#include <Types.h>

void SendRestock(Server* server, uint8 playerID);
void SendMoveObject(Server* server, uint8 object, uint8 team, Vector3f pos);
void SendIntelCapture(Server* server, uint8 playerID, uint8 winning);
void SendIntelPickup(Server* server, uint8 playerID);
void SendIntelDrop(Server* server, uint8 playerID);
void SendGrenade(Server* server, uint8 playerID, float fuse, Vector3f position, Vector3f velocity);
void SendPlayerLeft(Server* server, uint8 playerID);
void SendWeaponReload(Server* server, uint8 playerID, uint8 startAnimation, uint8 clip, uint8 reserve);
void SendWeaponInput(Server* server, uint8 playerID, uint8 wInput);
void SendSetColor(Server* server, uint8 playerID, uint8 R, uint8 G, uint8 B);
void SendSetTool(Server* server, uint8 playerID, uint8 tool);
void SendBlockLine(Server* server, uint8 playerID, Vector3i start, Vector3i end);
void SendBlockAction(Server* server, uint8 playerID, uint8 actionType, int X, int Y, int Z);
void SendStateData(Server* server, uint8 playerID);
void SendInputData(Server* server, uint8 playerID);
void sendKillPacket(Server* server,
                    uint8   killerID,
                    uint8   playerID,
                    uint8   killReason,
                    uint8   respawnTime,
                    uint8   makeInvisible);
void sendHP(Server*  server,
            uint8    playerID,
            uint8    hitPlayerID,
            long     HPChange,
            uint8    typeOfDamage,
            uint8    killReason,
            uint8    respawnTime,
            uint8    isGrenade,
            Vector3f position);
void SendPlayerState(Server* server, uint8 playerID, uint8 otherID);
void SendMapStart(Server* server, uint8 playerID);
void SendMapChunks(Server* server, uint8 playerID);
void SendRespawnState(Server* server, uint8 playerID, uint8 otherID);
void SendRespawn(Server* server, uint8 playerID);
void handleAndSendMessage(ENetEvent event, DataStream* data, Server* server, uint8 playerID);
void SendWorldUpdate(Server* server, uint8 playerID);
void SendPositionPacket(Server* server, uint8 playerID, float x, float y, float z);

void OnPacketReceived(Server* server, uint8 playerID, DataStream* data, ENetEvent event);

#endif
