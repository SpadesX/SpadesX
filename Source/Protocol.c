//Copyright DarkNeutrino 2021
#include <stdio.h>
#include <enet/enet.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <libvxl/libvxl.h>
#include <time.h>

#include "Enums.h"
#include "Server.h"
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

uint8 checkPlayerOnIntel(Server* server, uint8 playerID, uint8 team) {
	uint8 ret = 0;
	Vector3f playerPos = server->player[playerID].movement.position;
	Vector3f intelPos = server->protocol.ctf.intel[team];
	if ((int)playerPos.y == (int)intelPos.y && ((int)playerPos.z + 3 == (int)intelPos.z || (server->player[playerID].crouching && (int)playerPos.z + 2 == (int)intelPos.z)) && (int)playerPos.x == (int)intelPos.x) {
		ret = 1;
	}
	return ret;
}

uint8 checkPlayerInTent(Server* server, uint8 playerID) {
	uint8 ret = 0;
	Vector3f playerPos = server->player[playerID].movement.position;
	Vector3f tentPos = server->protocol.ctf.base[server->player[playerID].team];
	if (((int)playerPos.z + 3 == (int)tentPos.z || (server->player[playerID].crouching && (int)playerPos.z + 2 == (int)tentPos.z)) && ((int)playerPos.x >= (int)tentPos.x - 1 && (int)playerPos.x <= (int)tentPos.x) && ((int)playerPos.y >= (int)tentPos.y - 1 && (int)playerPos.y <= (int)tentPos.y)) {
		ret = 1;
	}
	return ret;
}

uint8 checkItemOnIntel(Server* server, uint8 team, Vector3f itemPos) {
	uint8 ret = 0;
	Vector3f intelPos = server->protocol.ctf.intel[team];
	if ((int)itemPos.y == (int)intelPos.y && ((int)itemPos.z == (int)intelPos.z) && (int)itemPos.x == (int)intelPos.x) {
		ret = 1;
	}
	return ret;
}

uint8 checkItemInTent(Server* server, uint8 team, Vector3f itemPos) {
	uint8 ret = 0;
	Vector3f tentPos = server->protocol.ctf.base[team];
	if (((int)itemPos.z == (int)tentPos.z) && ((int)itemPos.x >= (int)tentPos.x - 1 && (int)itemPos.x <= (int)tentPos.x) && ((int)itemPos.y >= (int)tentPos.y - 1 && (int)itemPos.y <= (int)tentPos.y)) {
		ret = 1;
	}
	return ret;
}

Vector3f SetIntelTentSpawnPoint(Server* server, uint8 team)
{
		Quad2D* spawn = server->protocol.spawns + team;

		float dx = spawn->to.x - spawn->from.x;
		float dy = spawn->to.y - spawn->from.y;
		Vector3f position;
		position.x = spawn->from.x + dx * ((float) rand() / (float) RAND_MAX);
		position.y = spawn->from.y + dy * ((float) rand() / (float) RAND_MAX);
		position.z = 62.f;
		return position;
}

void handleIntel(Server* server, uint8 playerID) {
	uint8 team;
	if (server->player[playerID].team == 0) {
		team = 1;
	}
	else {
		team = 0;
	}
	if (server->player[playerID].team != TEAM_SPECTATOR) {
		time_t timeNow = time(NULL);
		if (server->player[playerID].hasIntel == 0) {
			
			if (checkPlayerOnIntel(server, playerID, team) && (!server->protocol.ctf.intelHeld[team])) {
				SendIntelPickup(server, playerID);
				server->protocol.ctf.intelHeld[team] = 1;
				server->player[playerID].hasIntel = 1;
			}
			else if (checkPlayerInTent(server, playerID) && timeNow - server->player[playerID].sinceLastBaseEnterRestock >= 15) {
					SendRestock(server, playerID);
					server->player[playerID].sinceLastBaseEnterRestock = time(NULL);
			}
		}
		else if (server->player[playerID].hasIntel) {
			if (checkPlayerInTent(server, playerID) && timeNow - server->player[playerID].sinceLastBaseEnter >= 5) {
				server->protocol.ctf.score[server->player[playerID].team]++;
				uint8 winning = 0;
				if (server->protocol.ctf.score[server->player[playerID].team] >= server->protocol.ctf.scoreLimit) {
					winning = 1;
				}
				SendIntelCapture(server, playerID, winning);
				server->player[playerID].hasIntel = 0;
				server->protocol.ctf.intelHeld[team] = 0;
				server->player[playerID].sinceLastBaseEnter = time(NULL);
				server->protocol.ctf.intel[team] = SetIntelTentSpawnPoint(server, team);
				SendMoveObject(server, team, team, server->protocol.ctf.intel[team]);
				if (winning) {
					for (uint32 i = 0; i < server->protocol.maxPlayers; ++i) {
						if (server->player[i].state != STATE_DISCONNECTED) {
						server->player[i].state = STATE_STARTING_MAP;
						}
					}
					ServerReset(server, "hallway.vxl");
				}
			}
		}
	}
}

void handleGrenade(Server* server, uint8 playerID) {
	for (int i = 0; i < 3; ++i) {
		if (server->player[playerID].grenade[i].sent) {
			move_grenade(server, playerID, i);
			if ((get_nanos() - server->player[playerID].grenade[i].timeSinceSent) / 1000000000.f >= server->player[playerID].grenade[i].fuse) {
				SendBlockAction(server, playerID, 3, server->player[playerID].grenade[i].position.x, server->player[playerID].grenade[i].position.y, server->player[playerID].grenade[i].position.z);
				for (int y = 0; y < server->protocol.maxPlayers; ++y) {
					if (server->player[y].state == STATE_READY) {
						int diffX = fabsf(server->player[y].movement.position.x - server->player[playerID].grenade[i].position.x);
						int diffY = fabsf(server->player[y].movement.position.y - server->player[playerID].grenade[i].position.y);
						int diffZ = fabsf(server->player[y].movement.position.z - server->player[playerID].grenade[i].position.z);
						Vector3f playerPos = server->player[y].movement.position;
						Vector3f grenadePos = server->player[y].grenade[i].position;
						if (diffX < 16 && diffY < 16 && diffZ <16 && can_see(server, playerPos.x, playerPos.y, playerPos.z, grenadePos.x, grenadePos.y, grenadePos.z) && server->player[playerID].grenade[i].position.z < 62) {
							int value = 4096 / ((diffX*diffX) + (diffY*diffY) + (diffZ*diffZ));
							sendHP(server, playerID, y, value, 1, 3, 5);
						}
					}
				}
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

void updateMovementAndGrenades(Server *server, unsigned long long timeNow, unsigned long long timeSinceLastUpdate, unsigned long long timeSinceStart) {
	set_globals((timeNow - timeSinceStart)/1000000000.f, (timeNow - timeSinceLastUpdate) / 1000000000.f);
	for (int playerID = 0; playerID < server->protocol.maxPlayers; playerID++) {
		if (server->player[playerID].state == STATE_READY) {
			long falldamage = 0;
			falldamage = move_player(server, playerID);
			if (falldamage > 0) {
				sendHP(server, playerID, playerID, falldamage, 0, 4, 5);
			}
			handleGrenade(server, playerID);
			handleIntel(server, playerID);
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
