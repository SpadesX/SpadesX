#include <stdio.h>
#include <enet/enet.h>

#include "Structs.h"
#include "DataStream.h"
#include "Types.h"

void SendPlayerState(Server* server, uint8 playerID, uint8 otherID)
{
	ENetPacket* packet = enet_packet_create(NULL, 32, ENET_PACKET_FLAG_RELIABLE);
	DataStream  stream = {packet->data, packet->dataLength, 0};
	WriteByte(&stream, PACKET_TYPE_EXISTING_PLAYER);
	WriteByte(&stream, otherID);					// ID
	WriteByte(&stream, server->player[otherID].team);	  // TEAM
	WriteByte(&stream, server->player[otherID].weapon);	// WEAPON
	WriteByte(&stream, server->player[otherID].item);	//HELD ITEM
	WriteInt(&stream, server->player[otherID].kills);	//KILLS
	WriteColor3i(&stream, server->player[otherID].color);	//COLOR
	WriteArray(&stream, server->player[otherID].name, 16); // NAME

	if (enet_peer_send(server->player[playerID].peer, 0, packet) != 0) {
		WARNING("failed to send player state\n");
	}
}
