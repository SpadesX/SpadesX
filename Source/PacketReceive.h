#ifndef PACKETRECEIVE_H
#define PACKETRECEIVE_H

#include <enet/enet.h>
#include "Structs.h"
#include "Types.h"
#include "DataStream.h"

void ReceiveHitPacket(Server* server, uint8 playerID, uint8 hitPlayerID, uint8 hitType);
void ReceiveOrientationData(Server* server, uint8 playerID, DataStream* data);
void ReceiveInputData(Server* server, uint8 playerID, DataStream* data);
void ReceivePositionData(Server* server, uint8 playerID, DataStream* data);
void ReceiveExistingPlayer(Server* server, uint8 playerID, DataStream* data);
void OnPacketReceived(Server* server, uint8 playerID, DataStream* data, ENetEvent event);

#endif
