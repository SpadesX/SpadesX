#ifndef CONVERSION_H
#define CONVERSION_H

static void uint32ToUint8(Server* server, ENetEvent event, uint8 playerID) {
	uint32 ip = event.peer->address.host;
	server->player[playerID].ip[1] = (uint32)(ip & 0xff);
	server->player[playerID].ip[2] = (uint32)(ip >> 8) & 0xff;
	server->player[playerID].ip[3] = (uint32)(ip >> 16) & 0xff;
	server->player[playerID].ip[4] = (uint32)(ip >> 24);
}

#endif
