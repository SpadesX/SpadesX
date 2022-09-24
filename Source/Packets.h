// Copyright DarkNeutrino 2021
#ifndef PACKETS_H
#define PACKETS_H

#include "Structs.h"

#include "Util/DataStream.h"
#include "Util/Line.h"
#include "Util/Queue.h"
#include "Util/Types.h"

void sendRestock(Server* server, uint8 playerID);
void sendMoveObject(Server* server, uint8 object, uint8 team, Vector3f pos);
void sendIntelCapture(Server* server, uint8 playerID, uint8 winning);
void sendIntelPickup(Server* server, uint8 playerID);
void sendIntelDrop(Server* server, uint8 playerID);
void sendGrenade(Server* server, uint8 playerID, float fuse, Vector3f position, Vector3f velocity);
void sendPlayerLeft(Server* server, uint8 playerID);
void sendWeaponReload(Server* server, uint8 playerID, uint8 startAnimation, uint8 clip, uint8 reserve);
void sendWeaponInput(Server* server, uint8 playerID, uint8 wInput);
void sendSetColor(Server* server, uint8 playerID, uint8 R, uint8 G, uint8 B);
void sendSetColorToPlayer(Server* server, uint8 playerID, uint8 sendToID, uint8 R, uint8 G, uint8 B);
void sendSetTool(Server* server, uint8 playerID, uint8 tool);
void sendBlockLine(Server* server, uint8 playerID, Vector3i start, Vector3i end);
void sendBlockLineToPlayer(Server* server, uint8 playerID, uint8 sendToID, Vector3i start, Vector3i end);
void sendBlockAction(Server* server, uint8 playerID, uint8 actionType, int X, int Y, int Z);
void sendBlockActionToPlayer(Server* server, uint8 playerID, uint8 sendToID, uint8 actionType, int X, int Y, int Z);
void sendStateData(Server* server, uint8 playerID);
void sendInputData(Server* server, uint8 playerID);
void sendKillActionPacket(Server* server,
                    uint8   killerID,
                    uint8   playerID,
                    uint8   killReason,
                    uint8   respawnTime,
                    uint8   makeInvisible);
void sendSetHP(Server*  server,
            uint8    playerID,
            uint8    hitPlayerID,
            long     HPChange,
            uint8    typeOfDamage,
            uint8    killReason,
            uint8    respawnTime,
            uint8    isGrenade,
            Vector3f position);
void sendExistingPlayer(Server* server, uint8 playerID, uint8 otherID);
void sendMapStart(Server* server, uint8 playerID);
void sendMapChunks(Server* server, uint8 playerID);
void sendCreatePlayer(Server* server, uint8 playerID, uint8 otherID);
void SendRespawn(Server* server, uint8 playerID);
void handleAndSendMessage(ENetEvent event, DataStream* data, Server* server, uint8 playerID);
void SendWorldUpdate(Server* server, uint8 playerID);
void SendPositionPacket(Server* server, uint8 playerID, float x, float y, float z);

void OnPacketReceived(Server* server, uint8 playerID, DataStream* data, ENetEvent event);

#endif
