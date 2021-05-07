//Copyright DarkNeutrino 2021
#include <stdio.h>
#include <enet/enet.h>

#include "Structs.h"
#include "DataStream.h"
#include "Types.h"

void SendPacketExceptSender(Server *server, ENetPacket* packet, uint8 playerID) {
	for (uint8 i = 0; i < 32; ++i) {
		if (playerID != i && server->player[i].state != STATE_DISCONNECTED) {
			enet_peer_send(server->player[i].peer, 0, packet);
		}
	}
}

void SetTeamKillingFlag(Server *server, uint8 flag) {
	for (uint8 i = 0; i < 32; ++i) {
		server->player[i].allowTK = flag;
	}
}
