//Copyright DarkNeutrino 2021
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "Enums.h"
#include "Queue.h"
#include "Types.h"
#include "Map.h"
#include "DataStream.h"

#include <enet/enet.h>

void handleGrenade(Server* server, uint8 playerID);
void updateMovementAndGrenades(Server* server, unsigned long long timeNow, unsigned long long timeSinceLastUpdate, unsigned long long timeSinceStart);
void SetPlayerRespawnPoint(Server* server, uint8 playerID);
Vector3f SetIntelTentSpawnPoint(Server* server, uint8 team);
void sendServerNotice(Server* server, uint8 playerID, char *message);
void broadcastServerNotice(Server* server, char *message);
void SendPacketExceptSender(Server* server, ENetPacket* packet, uint8 playerID);
void SendPacketExceptSenderDistCheck(Server* server, ENetPacket* packet, uint8 playerID);
uint8 playerToPlayerVisible(Server *server, uint8 playerID, uint8 playerID2);

#endif
