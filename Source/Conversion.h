//Copyright DarkNeutrino 2021
#ifndef CONVERSION_H
#define CONVERSION_H

#include <enet/enet.h>
#include "Structs.h"
#include "Types.h"


void uint32ToUint8(Server* server, ENetEvent event, uint8 playerID) {
	uint32 ip = event.peer->address.host;
	server->player[playerID].ip[0] = (uint32)(ip & 0xff);
	server->player[playerID].ip[1] = (uint32)(ip >> 8) & 0xff;
	server->player[playerID].ip[2] = (uint32)(ip >> 16) & 0xff;
	server->player[playerID].ip[3] = (uint32)(ip >> 24);
}

int color4iToInt(Color4i color) {
	int intColor = 0;
	intColor = ((uint64)(((uint8)color[0]) << 24) |  (uint64)(((uint8)color[1]) << 16) |  (uint64)(((uint8)color[2]) << 8) | (uint64)((uint8)color[3]));
	return intColor;
}

#endif
