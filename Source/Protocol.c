//Copyright DarkNeutrino 2021
#include <stdio.h>
#include <enet/enet.h>
#include <math.h>

#include "Enums.h"
#include "Structs.h"
#include "DataStream.h"
#include "Types.h"

uint8 playerToPlayerVisible(Server *server, uint8 playerID, uint8 playerID2) {
	float distance = 0;
	distance = sqrt(pow((server->player[playerID].pos.x - server->player[playerID2].pos.x), 2) + pow((server->player[playerID].pos.y - server->player[playerID2].pos.y), 2));
	if (server->player[playerID2].team == TEAM_SPECTATOR) {
		return 1;
	}
	else if (distance >= 132) {
		return 0;
	}
	else {
		return 1;
	}
}

void SendPacketExceptSender(Server *server, ENetPacket* packet, uint8 playerID) {
	for (uint8 i = 0; i < 32; ++i) {
		if (playerID != i && server->player[i].state != STATE_DISCONNECTED) {
			enet_peer_send(server->player[i].peer, 0, packet);
		}
	}
}

void SendPacketExceptSenderDistCheck(Server *server, ENetPacket* packet, uint8 playerID) {
	for (uint8 i = 0; i < 32; ++i) {
		if (playerID != i && server->player[i].state != STATE_DISCONNECTED) {
			if (playerToPlayerVisible(server, playerID, i) || server->player[i].team == TEAM_SPECTATOR) {
				enet_peer_send(server->player[i].peer, 0, packet);
			}
		}
	}
}

void SetTeamKillingFlag(Server *server, uint8 flag) {
	for (uint8 i = 0; i < 32; ++i) {
		server->player[i].allowTK = flag;
	}
}
