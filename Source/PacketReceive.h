// Copyright DarkNeutrino 2021
#ifndef PACKETRECEIVE_H
#define PACKETRECEIVE_H

#include "Structs.h"

#include <DataStream.h>
#include <Types.h>
#include <enet/enet.h>

void OnPacketReceived(Server* server, uint8 playerID, DataStream* data, ENetEvent event);

#endif
