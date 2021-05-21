//Copyright DarkNeutrino 2021
#include <stdio.h>
#include <enet/enet.h>
#include <math.h>
#include <string.h>
#include <libvxl/libvxl.h>

#include "Enums.h"
#include "Structs.h"
#include "DataStream.h"
#include "Types.h"
#include "Packets.h"
#include "Physics.h"

static unsigned long long get_nanos(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (unsigned long long)ts.tv_sec * 1000000000L + ts.tv_nsec;
}

void updateMovementAndGrenades(Server *server, unsigned long long timeNow, unsigned long long timeSinceLastUpdate, unsigned long long timeSinceStart) {
	set_globals((timeNow - timeSinceStart)/1000000000.f, (timeNow - timeSinceLastUpdate) / 1000000000.f);
	for (int playerID = 0; playerID < server->protocol.maxPlayers; playerID++) {
		if (server->player[playerID].state == STATE_READY) {
			long falldamage = 0;
			falldamage = move_player(server, playerID);
			if (falldamage > 0) {
				sendHP(server, playerID, playerID, falldamage, 0, 4, 5);
			}
			for (int i = 0; i < 3; ++i) {
				if (server->player[playerID].grenade[i].sent) {
					move_grenade(server, playerID, i);
					if ((get_nanos() - server->player[playerID].grenade[i].timeSinceSent) / 1000000000.f >= server->player[playerID].grenade[i].fuse) {
						SendBlockAction(server, playerID, 3, server->player[playerID].grenade[i].position.x, server->player[playerID].grenade[i].position.y, server->player[playerID].grenade[i].position.z);
						for (int z = server->player[playerID].grenade[i].position.z - 1; z <= server->player[playerID].grenade[i].position.z + 1; ++z) {
							for (int y = server->player[playerID].grenade[i].position.y - 1; y <= server->player[playerID].grenade[i].position.y + 1; ++y) {
								for (int x = server->player[playerID].grenade[i].position.x - 1; x <= server->player[playerID].grenade[i].position.x + 1; ++x) {
									if (z < 62 && y >= 0 && y <= 512 && x <= 512 && x >= 0) {
										libvxl_map_setair(&server->map.map, x, y, z);
									}
								}
							}
						}
						server->player[playerID].grenade[i].sent = 0;
					}
				}
			}
		}
	}
}

void SetPlayerRespawnPoint(Server* server, uint8 playerID)
{
	if (server->player[playerID].team != TEAM_SPECTATOR) {
		Quad2D* spawn = server->protocol.spawns + server->player[playerID].team;

		float dx = spawn->to.x - spawn->from.x;
		float dy = spawn->to.y - spawn->from.y;

		server->player[playerID].movement.position.x = spawn->from.x + dx * ((float) rand() / (float) RAND_MAX);
		server->player[playerID].movement.position.y = spawn->from.y + dy * ((float) rand() / (float) RAND_MAX);
		server->player[playerID].movement.position.z = 62.f - 2.36f;

		server->player[playerID].movement.forwardOrientation.x = 0.f;
		server->player[playerID].movement.forwardOrientation.y = 0.f;
		server->player[playerID].movement.forwardOrientation.z = 0.f;

		printf("respawn: %f %f %f\n", server->player[playerID].movement.position.x, server->player[playerID].movement.position.y, server->player[playerID].movement.position.z);
	}
}

void sendServerNotice(Server* server, uint8 playerID, char *message) {
	uint32 packetSize = 3 + strlen(message);
	ENetPacket* packet = enet_packet_create(NULL, packetSize, ENET_PACKET_FLAG_RELIABLE);
	DataStream  stream = {packet->data, packet->dataLength, 0};
	WriteByte(&stream, PACKET_TYPE_CHAT_MESSAGE);
	WriteByte(&stream, 33);
	WriteByte(&stream, 0);
	WriteArray(&stream, message, strlen(message));
	enet_peer_send(server->player[playerID].peer, 0, packet);
}

void broadcastServerNotice(Server* server, char *message) {
	uint32 packetSize = 3 + strlen(message);
	ENetPacket* packet = enet_packet_create(NULL, packetSize, ENET_PACKET_FLAG_RELIABLE);
	DataStream  stream = {packet->data, packet->dataLength, 0};
	WriteByte(&stream, PACKET_TYPE_CHAT_MESSAGE);
	WriteByte(&stream, 33);
	WriteByte(&stream, 0);
	WriteArray(&stream, message, strlen(message));
	enet_host_broadcast(server->host, 0, packet);
}

uint8 playerToPlayerVisible(Server *server, uint8 playerID, uint8 playerID2) {
	float distance = 0;
	distance = sqrt(pow((server->player[playerID].movement.position.x - server->player[playerID2].movement.position.x), 2) + pow((server->player[playerID].movement.position.y - server->player[playerID2].movement.position.y), 2));
	if (server->player[playerID].team == TEAM_SPECTATOR) {
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
			if (playerToPlayerVisible(server, playerID, i)) {
				enet_peer_send(server->player[i].peer, 0, packet);
			}
		}
	}
}
