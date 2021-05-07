//Copyright DarkNeutrino 2021
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "Enums.h"
#include "Queue.h"
#include "Types.h"
#include "Map.h"
#include "DataStream.h"

#include <enet/enet.h>

void SendPacketExceptSender(Server* server, ENetPacket* packet, uint8 playerID);
void SetTeamKillingFlag(Server* server, uint8 flag);

#endif
