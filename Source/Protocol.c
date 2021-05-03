#include <enet/enet.h>

#include "Structs.h"
#include "DataStream.h"
#include "Types.h"


void SendStateData(Server* server, uint8 playerID)
{
	ENetPacket* packet = enet_packet_create(NULL, 104, ENET_PACKET_FLAG_RELIABLE);
	DataStream  stream = {packet->data, packet->dataLength, 0};
	WriteByte(&stream, PACKET_TYPE_STATE_DATA);
	WriteByte(&stream, playerID);
	WriteColor3i(&stream, server->protocol.colorFog);
	WriteColor3i(&stream, server->protocol.colorTeamA);
	WriteColor3i(&stream, server->protocol.colorTeamB);
	WriteArray(&stream, server->protocol.nameTeamA, 10);
	WriteArray(&stream, server->protocol.nameTeamB, 10);
	WriteByte(&stream, server->protocol.mode);

	// MODE CTF:

	WriteByte(&stream, server->protocol.ctf.scoreTeamA); // SCORE TEAM A
	WriteByte(&stream, server->protocol.ctf.scoreTeamB); // SCORE TEAM B
	WriteByte(&stream, server->protocol.ctf.scoreLimit); // SCORE LIMIT
	WriteByte(&stream, server->protocol.ctf.intelFlags); // INTEL FLAGS

	if ((server->protocol.ctf.intelFlags & 1) == 0) {
		WriteVector3f(&stream, &server->protocol.ctf.intelTeamA);
	} else {
		WriteByte(&stream, server->protocol.ctf.playerIntelTeamA);
		StreamSkip(&stream, 11);
	}

	if ((server->protocol.ctf.intelFlags & 2) == 0) {
		WriteVector3f(&stream, &server->protocol.ctf.intelTeamB);
	} else {
		WriteByte(&stream, server->protocol.ctf.playerIntelTeamB);
		StreamSkip(&stream, 11);
	}

	WriteVector3f(&stream, &server->protocol.ctf.baseTeamA);
	WriteVector3f(&stream, &server->protocol.ctf.baseTeamB);

	if (enet_peer_send(server->player[playerID].peer, 0, packet) == 0) {
		server->player[playerID].state = STATE_READY;
	}
}
