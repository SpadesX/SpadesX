#include "Server.h"

#include "Enums.h"
#include "Types.h"
#include "Line.h"

#include <Compress.h>
#include <DataStream.h>
#include <Queue.h>
#include <enet/enet.h>
#include <libvxl/libvxl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct
{
	uint8	 scoreTeamA;
	uint8	 scoreTeamB;
	uint8	 scoreLimit;
	IntelFlag intelFlags;
	Vector3f  intelTeamA;
	Vector3f  intelTeamB;
	Vector3f  baseTeamA;
	Vector3f  baseTeamB;
	uint8	 playerIntelTeamA; // player ID if intel flags & 1 != 0
	uint8	 playerIntelTeamB; // player ID if intel flags & 2 != 0
} ModeCTF;

typedef struct
{
	ENetHost* host;
	//Master
	ENetHost* client;
	ENetPeer* masterpeer;
	uint8 countOfUsers;
	uint8 enableMasterConnection;
	time_t waitBeforeSend;
	//
	uint8 numPlayers;
	uint8 maxPlayers;
	//
	Color3i  colorFog;
	Color3i  colorTeamA;
	Color3i  colorTeamB;
	char	 nameTeamA[11];
	char	 nameTeamB[11];
	GameMode mode;
	// mode
	ModeCTF ctf;
	// compressed map
	Queue* compressedMap;
	uint32 compressedSize;
	uint32 mapSize;
	// respawn area
	Quad2D spawns[2];
	// dirty flag
	uint8 dirty;
	// per player
	uint8  input[32];
	uint32 inputFlags;
	struct libvxl_map map;

	Queue*		queues[32];
	State		state[32];
	Weapon		weapon[32];
	Team		team[32];
	Tool		item[32];
	uint32		kills[32];
	Color3i		color[32];
	Vector3f	pos[32];
	Vector3f	rot[32];
	char		name[17][32];
	ENetPeer*	peer[32];
	uint8		respawnTime[32];
	uint32		startOfRespawnWait[32];
	uint8		HP[32];
	uint64		toolColor[32];
	uint8		weaponReserve[32];
	short		weaponClip[32];
	vec3i		resultLine[32][50];
	uint8		ip[32][4];
} GameServer;

#define STATUS(msg)  printf("STATUS: " msg "\n")
#define WARNING(msg) printf("WARNING: " msg "\n")
#define ERROR(msg)   fprintf(stderr, "ERROR: " msg "\n");

static void SetPlayerRespawnPoint(GameServer* server, uint8 playerID)
{
	if (server->team[playerID] != TEAM_SPECTATOR) {
		Quad2D* spawn = server->spawns + server->team[playerID];

		float dx = spawn->to.x - spawn->from.x;
		float dy = spawn->to.y - spawn->from.y;

		server->pos[playerID].x = spawn->from.x + dx * ((float) rand() / (float) RAND_MAX);
		server->pos[playerID].y = spawn->from.y + dy * ((float) rand() / (float) RAND_MAX);
		server->pos[playerID].z = 62.f;

		server->rot[playerID].x = 0.f;
		server->rot[playerID].y = 0.f;
		server->rot[playerID].z = 0.f;

		printf("respawn: %f %f %f\n", server->pos[playerID].x, server->pos[playerID].y, server->pos[playerID].z);
	}
}

//
// On player connection:
//
// START_MAP
// MAP_CHUNK multiple
// STATE_DATA
// CREATE_PLAYER multiple
//

static void uint32ToUint8(GameServer* server, ENetEvent event, uint8 playerID) {
	unsigned long x = event.peer->address.host;  // you need UL to indicate it is unsigned long
	server->ip[playerID][1] = (unsigned int)(x & 0xff);
	server->ip[playerID][2] = (unsigned int)(x >> 8) & 0xff;
	server->ip[playerID][3] = (unsigned int)(x >> 16) & 0xff;
	server->ip[playerID][4] = (unsigned int)(x >> 24);
}

static void SendMapStart(GameServer* server, uint8 playerID)
{
	STATUS("sending map info");
	ENetPacket* packet = enet_packet_create(NULL, 5, ENET_PACKET_FLAG_RELIABLE);
	DataStream  stream = {packet->data, packet->dataLength, 0};
	WriteByte(&stream, PACKET_TYPE_MAP_START);
	WriteInt(&stream, server->compressedSize);
	if (enet_peer_send(server->peer[playerID], 0, packet) == 0) {
		server->state[playerID] = STATE_LOADING_CHUNKS;
		
		// map
		uint8* mapOut = (uint8*) malloc(server->mapSize);
		libvxl_write(&server->map, mapOut, &server->mapSize);
		server->compressedMap = CompressData(mapOut, server->mapSize, DEFAULT_COMPRESSOR_CHUNK_SIZE);
		server->queues[playerID] = server->compressedMap;
		//free(mapOut); //Causes double free so lets comment it out
	}
}

static void SendMapChunks(GameServer* server, uint8 playerID)
{
	if (server->queues[playerID] == NULL) {
		server->state[playerID] = STATE_JOINING;
		STATUS("loading chunks done");
	} else {
		ENetPacket* packet = enet_packet_create(NULL, server->queues[playerID]->length + 1, ENET_PACKET_FLAG_RELIABLE);
		DataStream  stream = {packet->data, packet->dataLength, 0};
		WriteByte(&stream, PACKET_TYPE_MAP_CHUNK);
		WriteArray(&stream, server->queues[playerID]->block, server->queues[playerID]->length);

		if (enet_peer_send(server->peer[playerID], 0, packet) == 0) {
			server->queues[playerID] = server->queues[playerID]->next;
		}
	}
}

static void SendStateData(GameServer* server, uint8 playerID)
{
	ENetPacket* packet = enet_packet_create(NULL, 104, ENET_PACKET_FLAG_RELIABLE);
	DataStream  stream = {packet->data, packet->dataLength, 0};
	WriteByte(&stream, PACKET_TYPE_STATE_DATA);
	WriteByte(&stream, playerID);
	WriteColor3i(&stream, server->colorFog);
	WriteColor3i(&stream, server->colorTeamA);
	WriteColor3i(&stream, server->colorTeamB);
	WriteArray(&stream, server->nameTeamA, 10);
	WriteArray(&stream, server->nameTeamB, 10);
	WriteByte(&stream, server->mode);

	// MODE CTF:

	WriteByte(&stream, server->ctf.scoreTeamA); // SCORE TEAM A
	WriteByte(&stream, server->ctf.scoreTeamB); // SCORE TEAM B
	WriteByte(&stream, server->ctf.scoreLimit); // SCORE LIMIT
	WriteByte(&stream, server->ctf.intelFlags); // INTEL FLAGS

	if ((server->ctf.intelFlags & 1) == 0) {
		WriteVector3f(&stream, &server->ctf.intelTeamA);
	} else {
		WriteByte(&stream, server->ctf.playerIntelTeamA);
		StreamSkip(&stream, 11);
	}

	if ((server->ctf.intelFlags & 2) == 0) {
		WriteVector3f(&stream, &server->ctf.intelTeamB);
	} else {
		WriteByte(&stream, server->ctf.playerIntelTeamB);
		StreamSkip(&stream, 11);
	}

	WriteVector3f(&stream, &server->ctf.baseTeamA);
	WriteVector3f(&stream, &server->ctf.baseTeamB);

	if (enet_peer_send(server->peer[playerID], 0, packet) == 0) {
		server->state[playerID] = STATE_READY;
	}
}

static void SendPlayerState(GameServer* server, uint8 playerID, uint8 otherID) //Change me to existing player.
{
	ENetPacket* packet = enet_packet_create(NULL, 32, ENET_PACKET_FLAG_RELIABLE);
	DataStream  stream = {packet->data, packet->dataLength, 0};
	WriteByte(&stream, PACKET_TYPE_EXISTING_PLAYER);
	WriteByte(&stream, otherID);					// ID
	WriteByte(&stream, server->team[otherID]);	  // TEAM
	WriteByte(&stream, server->weapon[otherID]);	// WEAPON
	WriteByte(&stream, server->item[otherID]);	//HELD ITEM
	WriteInt(&stream, server->kills[otherID]);	//KILLS
	WriteColor3i(&stream, server->color[otherID]);	//COLOR
	WriteArray(&stream, server->name[otherID], 16); // NAME

	if (enet_peer_send(server->peer[playerID], 0, packet) != 0) {
		WARNING("failed to send player state\n");
	}
}

static void SendRespawnState(GameServer* server, uint8 playerID, uint8 otherID)
{
	ENetPacket* packet = enet_packet_create(NULL, 32, ENET_PACKET_FLAG_RELIABLE);
	DataStream  stream = {packet->data, packet->dataLength, 0};
	WriteByte(&stream, PACKET_TYPE_CREATE_PLAYER);
	WriteByte(&stream, otherID);					// ID
	WriteByte(&stream, server->weapon[otherID]);	// WEAPON
	WriteByte(&stream, server->team[otherID]);	  // TEAM
	WriteVector3f(&stream, server->pos + otherID);  // X Y Z
	WriteArray(&stream, server->name[otherID], 16); // NAME

	if (enet_peer_send(server->peer[playerID], 0, packet) != 0) {
		WARNING("failed to send player state\n");
	}
}

static void SendJoiningData(GameServer* server, uint8 playerID)
{
	STATUS("sending state");
	SendStateData(server, playerID);
	for (uint8 i = 0; i < server->maxPlayers; ++i) {
		if (i != playerID && server->state[i] != STATE_DISCONNECTED) {
			SendPlayerState(server, playerID, i);
		}
	}
}

static void SendPlayerLeft(GameServer* server, uint8 playerID)
{
	STATUS("sending player left event");
	for (uint8 i = 0; i < server->maxPlayers; ++i) {
		if (i != playerID && server->state[i] != STATE_DISCONNECTED) {
			ENetPacket* packet = enet_packet_create(NULL, 2, ENET_PACKET_FLAG_RELIABLE);
			packet->data[0]	= PACKET_TYPE_PLAYER_LEFT;
			packet->data[1]	= playerID;

			if (enet_peer_send(server->peer[i], 0, packet) != 0) {
				WARNING("failed to send player left event\n");
			}
		}
	}
}

static void SendRespawn(GameServer* server, uint8 playerID)
{
	for (uint8 i = 0; i < server->maxPlayers; ++i) {
		if (server->state[i] != STATE_DISCONNECTED) {
			SendRespawnState(server, i, playerID);
		}
	}
	server->state[playerID] = STATE_READY;
}

static uint8 OnConnect(GameServer* server, uint32 data)
{
	if (server->numPlayers == server->maxPlayers) {
		return 0xFF;
	}
	uint8 playerID;
	for (playerID = 0; playerID < server->maxPlayers; ++playerID) {
		if (server->state[playerID] == STATE_DISCONNECTED) {
			server->state[playerID] = STATE_STARTING_MAP;
			break;
		}
	}
	server->numPlayers++;
	return playerID;
}

static void SendInputData(GameServer* server, uint8 playerID)
{
	ENetPacket* packet = enet_packet_create(NULL, 3, ENET_PACKET_FLAG_RELIABLE);
	DataStream  stream = {packet->data, packet->dataLength, 0};
	WriteByte(&stream, PACKET_TYPE_INPUT_DATA);
	WriteByte(&stream, playerID);
	WriteByte(&stream, server->input[playerID]);
	for (uint8 i = 0; i < 32; ++i) {
		if (playerID != i && server->state[i] != STATE_DISCONNECTED) {
			enet_peer_send(server->peer[i], 0, packet);
		}
	}
}

static void ReceiveExistingPlayer(GameServer* server, uint8 playerID, DataStream* data)
{
	uint8 id = ReadByte(data);
	if (playerID != id) {
		WARNING("different player id");
	}

	server->team[playerID]   = ReadByte(data);
	server->weapon[playerID] = ReadByte(data);
	server->item[playerID]   = ReadByte(data);
	server->kills[playerID]  = ReadInt(data);

	ReadColor3i(data, server->color[playerID]);

	uint32 length = DataLeft(data);
	if (length > 16) {
		WARNING("name too long");
	} else {
		server->name[playerID][length] = '\0';
		ReadArray(data, server->name[playerID], length);
	}
	switch (server->weapon[playerID]) {
		case 0:
			server->weaponReserve[playerID] = 50;
			server->weaponClip[playerID] = 10;
			break;
		case 1:
			server->weaponReserve[playerID] = 120;
			server->weaponClip[playerID] = 30;
			break;
		case 2:
			server->weaponReserve[playerID] = 48;
			server->weaponClip[playerID] = 6;
			break;
	}
	server->state[playerID] = STATE_SPAWNING;
}

static void ReceivePositionData(GameServer* server, uint8 playerID, DataStream* data)
{
	server->pos[playerID].x = ReadFloat(data);
	server->pos[playerID].y = ReadFloat(data);
	server->pos[playerID].z = ReadFloat(data);
	server->dirty		   = 1;
}

static void ReceiveOrientationData(GameServer* server, uint8 playerID, DataStream* data)
{
	server->rot[playerID].x = ReadFloat(data);
	server->rot[playerID].y = ReadFloat(data);
	server->rot[playerID].z = ReadFloat(data);
	server->dirty		   = 1;
}

static void ReceiveInputData(GameServer* server, uint8 playerID, DataStream* data)
{
	StreamSkip(data, 1); // ID
	server->input[playerID] = ReadByte(data);
	SendInputData(server, playerID);
}

static void SendWorldUpdate(GameServer* server)
{
	ENetPacket* packet = enet_packet_create(NULL, 1 + (32 * 24), ENET_PACKET_FLAG_UNSEQUENCED);
	DataStream  stream = {packet->data, packet->dataLength, 0};
	WriteByte(&stream, PACKET_TYPE_WORLD_UPDATE);

	for (uint8 j = 0; j < 32; ++j) {
		WriteVector3f(&stream, server->pos + j);
		WriteVector3f(&stream, server->rot + j);
	}

	enet_host_broadcast(server->host, 0, packet);
}

static void sendKillPacket(GameServer* server, uint8 killerID, uint8 playerID, uint8 killReason, uint8 respawnTime) {
	ENetPacket* packet = enet_packet_create(NULL, 5, ENET_PACKET_FLAG_RELIABLE);
	DataStream  stream = {packet->data, packet->dataLength, 0};
	WriteByte(&stream, PACKET_TYPE_KILL_ACTION);
	WriteByte(&stream, playerID); //Player that shot
	WriteByte(&stream, killerID); //playerID
	WriteByte(&stream, killReason); //Killing reason (1 is headshot)
	WriteByte(&stream, respawnTime); //Time before respawn happens
	for (uint8 i = 0; i < 32; ++i) {
		if (server->state[i] != STATE_DISCONNECTED) {
			enet_peer_send(server->peer[i], 0, packet);
		}
	}
	server->kills[killerID]++;
	server->respawnTime[playerID] = respawnTime;
	server->startOfRespawnWait[playerID] = time(NULL);
	server->kills[playerID]++;
	server->state[playerID] = STATE_WAITING_FOR_RESPAWN;
}

static void sendHP(GameServer* server, uint8 hitPlayerID, uint8 playerID, uint8 HPChange, uint8 type, uint8 killReason, uint8 respawnTime) {
	ENetPacket* packet = enet_packet_create(NULL, 15, ENET_PACKET_FLAG_RELIABLE);
	DataStream  stream = {packet->data, packet->dataLength, 0};
	server->HP[hitPlayerID] -= HPChange;
	if (server->HP[hitPlayerID] <= 0 || server->HP[hitPlayerID] >= 100) {
		//server->HP[hitPlayerID] = 100;
		sendKillPacket(server, hitPlayerID, playerID, killReason, respawnTime);
	}
	else {
	WriteByte(&stream, PACKET_TYPE_SET_HP);
	WriteByte(&stream, server->HP[hitPlayerID]);
	WriteByte(&stream, type);
	WriteFloat(&stream, server->pos[playerID].x);
	WriteFloat(&stream, server->pos[playerID].y);
	WriteFloat(&stream, server->pos[playerID].z);
	enet_peer_send(server->peer[playerID], 0, packet);
	}
}

static void sendMessage(ENetEvent event, DataStream* data, GameServer* server) {
	uint32 packetSize = event.packet->dataLength + 1;
	int player = ReadByte(data);
	int meantfor = ReadByte(data);
	uint32 length = DataLeft(data);
	char * message = calloc(length + 1, sizeof (char));
	ReadArray(data, message, length);
			message[length] = '\0';
			ENetPacket* packet = enet_packet_create(NULL, packetSize, ENET_PACKET_FLAG_RELIABLE);
			DataStream  stream = {packet->data, packet->dataLength, 0};
			WriteByte(&stream, PACKET_TYPE_CHAT_MESSAGE);
			WriteByte(&stream, player);
			WriteByte(&stream, meantfor);
			WriteArray(&stream, message, length);
			if (message[0] == '/') {
				if (message[1] == 'k' && message[2] == 'i' && message[3] == 'l' && message[4] == 'l') {
					sendKillPacket(server, player, player, 0, 5);
				}
			}
			else {
				enet_host_broadcast(server->host, 0, packet);
			}
			free(message);
}

static void updateMaster(GameServer* server) {
	server->countOfUsers = 0;
	for (int i = 0; i < 32; i++) {
		if (server->state[i] == STATE_READY) {
			server->countOfUsers++;
		}
	}
	ENetPacket* packet = enet_packet_create(NULL, 1, ENET_PACKET_FLAG_RELIABLE);
	DataStream  stream = {packet->data, packet->dataLength, 0};
	WriteByte(&stream, server->countOfUsers);
	enet_peer_send(server->masterpeer, 0, packet);
}

static void writeBlockLine(GameServer* server, uint8 playerID, vec3i* start, vec3i* end) {
	int size = blockLine(start, end, server->resultLine[playerID]);
	for (int i = 0; i < size; i++) {
		libvxl_map_set(&server->map, server->resultLine[playerID][i].x, server->resultLine[playerID][i].y, server->resultLine[playerID][i].z, server->toolColor[playerID]);
	}
}

static void OnPacketReceived(GameServer* server, uint8 playerID, DataStream* data, ENetEvent event)
{
	PacketID type = (PacketID) ReadByte(data);
	switch (type) {
		case PACKET_TYPE_EXISTING_PLAYER:
			ReceiveExistingPlayer(server, playerID, data);
			break;
		case PACKET_TYPE_POSITION_DATA:
			ReceivePositionData(server, playerID, data);
			break;
		case PACKET_TYPE_ORIENTATION_DATA:
			ReceiveOrientationData(server, playerID, data);
			break;
		case PACKET_TYPE_INPUT_DATA:
			ReceiveInputData(server, playerID, data);
			break;
		case PACKET_TYPE_CHAT_MESSAGE:
			sendMessage(event, data, server);
			break;
		case PACKET_TYPE_BLOCK_ACTION:
		{
			uint8 player = ReadByte(data);
			uint8 actionType = ReadByte(data);
			int X = ReadInt(data);
			int Y = ReadInt(data);
			int Z = ReadInt(data);
			switch (actionType) {
				case 0:
					libvxl_map_set(&server->map, X, Y, Z, server->toolColor[playerID]);
				break;
				
				case 1:
					libvxl_map_setair(&server->map, X, Y, Z);
				break;
				
				case 2:
					libvxl_map_setair(&server->map, X, Y, Z);
					libvxl_map_setair(&server->map, X, Y, Z-1);
					if (Z+1 < 62) {
						libvxl_map_setair(&server->map, X, Y, Z+1);
					}
				break;
			}
			ENetPacket* packet = enet_packet_create(NULL, 15, ENET_PACKET_FLAG_RELIABLE);
			DataStream  stream = {packet->data, packet->dataLength, 0};
			WriteByte(&stream, PACKET_TYPE_BLOCK_ACTION);
			WriteByte(&stream, player);
			WriteByte(&stream, actionType);
			WriteInt(&stream, X);
			WriteInt(&stream, Y);
			WriteInt(&stream, Z);
			enet_host_broadcast(server->host, 0, packet);
			break;
		}
		case PACKET_TYPE_BLOCK_LINE:
		{
			vec3i start;
			vec3i end;
			uint8 player = ReadByte(data);
			start.x = ReadInt(data);
			start.y = ReadInt(data);
			start.z = ReadInt(data);
			end.x = ReadInt(data);
			end.y = ReadInt(data);
			end.z = ReadInt(data);
			ENetPacket* packet = enet_packet_create(NULL, 26, ENET_PACKET_FLAG_RELIABLE);
			DataStream  stream = {packet->data, packet->dataLength, 0};
			WriteByte(&stream, PACKET_TYPE_BLOCK_LINE);
			WriteByte(&stream, player);
			WriteInt(&stream, start.x);
			WriteInt(&stream, start.y);
			WriteInt(&stream, start.z);
			WriteInt(&stream, end.x);
			WriteInt(&stream, end.y);
			WriteInt(&stream, end.z);
			enet_host_broadcast(server->host, 0, packet);
			writeBlockLine(server, playerID, &start, &end);
			break;
		}
		case PACKET_TYPE_SET_TOOL:
		{
			uint8 player = ReadByte(data);
			uint8 tool = ReadByte(data);
			ENetPacket* packet = enet_packet_create(NULL, 3, ENET_PACKET_FLAG_RELIABLE);
			DataStream  stream = {packet->data, packet->dataLength, 0};
			WriteByte(&stream, PACKET_TYPE_SET_TOOL);
			WriteByte(&stream, player);
			WriteByte(&stream, tool);
			for (uint8 i = 0; i < 32; ++i) {
				if (playerID != i && server->state[i] != STATE_DISCONNECTED) {
					enet_peer_send(server->peer[i], 0, packet);
				}
			}
			break;
		}
		case PACKET_TYPE_SET_COLOR:
		{
			uint8 player = ReadByte(data);
			uint8 B = ReadByte(data);
			uint8 G = ReadByte(data);
			uint8 R = ReadByte(data);
			uint8 A = 0;
			// I know looks horrible but simply we are putting 4 uint8 to uint64 in correct order
			server->toolColor[playerID] = ((uint64)(((uint8)A) << 24) |  (uint64)(((uint8)R) << 16) |  (uint64)(((uint8)G) << 8) | (uint64)((uint8)B));
			ENetPacket* packet = enet_packet_create(NULL, 5, ENET_PACKET_FLAG_RELIABLE);
			DataStream  stream = {packet->data, packet->dataLength, 0};
			WriteByte(&stream, PACKET_TYPE_SET_COLOR);
			WriteByte(&stream, player);
			WriteByte(&stream, B);
			WriteByte(&stream, G);
			WriteByte(&stream, R);
			for (uint8 i = 0; i < 32; ++i) {
				if (playerID != i && server->state[i] != STATE_DISCONNECTED) {
					enet_peer_send(server->peer[i], 0, packet);
				}
			}
			break;
		}
		
		case PACKET_TYPE_WEAPON_INPUT:
		{
			uint8 player = ReadByte(data);
			uint8 wInput = ReadByte(data);
			if (server->weaponClip[playerID] >= 0) {
				ENetPacket* packet = enet_packet_create(NULL, 3, ENET_PACKET_FLAG_RELIABLE);
				DataStream  stream = {packet->data, packet->dataLength, 0};
				WriteByte(&stream, PACKET_TYPE_WEAPON_INPUT);
				WriteByte(&stream, player);
				WriteByte(&stream, wInput);
				for (uint8 i = 0; i < 32; ++i) {
					if (playerID != i && server->state[i] != STATE_DISCONNECTED) {
						enet_peer_send(server->peer[i], 0, packet);
					}
				}
				if (wInput == 1 || wInput == 3) {
					server->weaponClip[playerID]--;
				}
			}
			else {
				//sendKillPacket(server, playerID, playerID, 0, 30);
			}
			break;
		}
		
		case PACKET_TYPE_HIT_PACKET:
		{
			uint8 hitPlayerID = ReadByte(data);
			Hit hitType = ReadByte(data);
			switch (server->weapon[playerID]) {
				case WEAPON_RIFLE:
				{
					switch (hitType) {
						case HIT_TYPE_HEAD:
						{
							sendKillPacket(server, playerID, hitPlayerID, 1, 5);
							break;
						}
						case HIT_TYPE_TORSO:
						{
							sendHP(server, playerID, hitPlayerID, 49, 1, 0, 5);
							break;
						}
						case HIT_TYPE_ARMS:
						{
							sendHP(server, playerID, hitPlayerID, 33, 1, 0, 5);
							break;
						}
						case HIT_TYPE_LEGS:
						{
							sendHP(server, playerID, hitPlayerID, 33, 1, 0, 5);
							break;
						}
					}
					break;
				}
				case WEAPON_SMG:
				{
					switch (hitType) {
						case HIT_TYPE_HEAD:
						{
							sendHP(server, playerID, hitPlayerID, 75, 1, 1, 5);
							break;
						}
						case HIT_TYPE_TORSO:
						{
							sendHP(server, playerID, hitPlayerID, 29, 1, 0, 5);
							break;
						}
						case HIT_TYPE_ARMS:
						{
							sendHP(server, playerID, hitPlayerID, 18, 1, 0, 5);
							break;
						}
						case HIT_TYPE_LEGS:
						{
							sendHP(server, playerID, hitPlayerID, 18, 1, 0, 5);
							break;
						}
					}
					break;
				}
				case WEAPON_SHOTGUN:
				{
					switch (hitType) {
						case HIT_TYPE_HEAD:
						{
							sendHP(server, playerID, hitPlayerID, 37, 1, 1, 5);
							break;
						}
						case HIT_TYPE_TORSO:
						{
							sendHP(server, playerID, hitPlayerID, 27, 1, 0, 5);
							break;
						}
						case HIT_TYPE_ARMS:
						{
							sendHP(server, playerID, hitPlayerID, 16, 1, 0, 5);
							break;
						}
						case HIT_TYPE_LEGS:
						{
							sendHP(server, playerID, hitPlayerID, 16, 1, 0, 5);
							break;
						}
					}
					break;
				}
			}
			break;
		}
		case PACKET_TYPE_WEAPON_RELOAD:
		{
			uint8 player = ReadByte(data);
			uint8 reserve = ReadByte(data);
			uint8 clip = ReadByte(data);
			server->weaponReserve[playerID] = 50; //Temporary
			server->weaponClip[playerID] = 10;
			ENetPacket* packet = enet_packet_create(NULL, 4, ENET_PACKET_FLAG_RELIABLE);
			DataStream  stream = {packet->data, packet->dataLength, 0};
			WriteByte(&stream, PACKET_TYPE_WEAPON_RELOAD);
			WriteByte(&stream, player);
			WriteByte(&stream, server->weaponReserve[playerID]);
			WriteByte(&stream, server->weaponClip[playerID]);
			for (uint8 i = 0; i < 32; ++i) {
				if (playerID != i && server->state[i] != STATE_DISCONNECTED) {
					enet_peer_send(server->peer[i], 0, packet);
				}
			}
			break;
		}
		case PACKET_TYPE_CHANGE_TEAM:
		{
			uint8 player = ReadByte(data);
			server->team[player] = ReadByte(data);
			sendKillPacket(server, playerID, playerID, 5, 5);
			server->state[playerID] = STATE_WAITING_FOR_RESPAWN;
			break;
		}
		case PACKET_TYPE_CHANGE_WEAPON:
		{
			uint8 player = ReadByte(data);
			server->weapon[player] = ReadByte(data);
			sendKillPacket(server, playerID, playerID, 6, 5);
			server->state[playerID] = STATE_WAITING_FOR_RESPAWN;
			break;
		}
		default:
			printf("unhandled input, id %u, code %u\n", playerID, type);
			break;
	}
}

static void OnPlayerUpdate(GameServer* server, uint8 playerID)
{
	switch (server->state[playerID]) {
		case STATE_STARTING_MAP:
			SendMapStart(server, playerID);
			break;
		case STATE_LOADING_CHUNKS:
			SendMapChunks(server, playerID);
			break;
		case STATE_JOINING:
			SendJoiningData(server, playerID);
			break;
		case STATE_SPAWNING:
			server->HP[playerID] = 100;
			SetPlayerRespawnPoint(server, playerID);
			SendRespawn(server, playerID);
			break;
		case STATE_WAITING_FOR_RESPAWN:
		{
			if (time(NULL) - server->startOfRespawnWait[playerID] >= server->respawnTime[playerID]) {
				server->state[playerID] = STATE_SPAWNING;
			}
			break;
		}
		case STATE_READY:
			// send data
			if (server->enableMasterConnection == 1) {
				updateMaster(server);
			}
			break;
		default:
			// disconnected
			break;
	}
}

static void ServerUpdate(GameServer* server, int timeout)
{
	ENetEvent event;
	if (server->enableMasterConnection == 1) {
		if (time(NULL) - server->waitBeforeSend >= 1) {
		ENetEvent eventMaster;
		while (enet_host_service(server->client, &eventMaster, 1) > 0) {
		//Quite literally here to keep the connection alive
		}
		server->waitBeforeSend = time(NULL);
		}
	}

	while (enet_host_service(server->host, &event, timeout) > 0) {
		uint8 playerID;
		switch (event.type) {
			case ENET_EVENT_TYPE_NONE:
				WARNING("none event");
				break;
			case ENET_EVENT_TYPE_CONNECT:
				if (event.data != VERSION_0_75) {
					enet_peer_disconnect(event.peer, REASON_WRONG_PROTOCOL_VERSION);
					break;
				}
				// check peer
				// ...
				// find next free ID
				playerID = OnConnect(server, event.data);
				if (playerID == 0xFF) {
					enet_peer_disconnect(event.peer, REASON_SERVER_FULL);
					break;
				}
				server->peer[playerID] = event.peer;
				event.peer->data	   = (void*) ((size_t) playerID);
				server->HP[playerID] = 100;
				uint32ToUint8(server, event, playerID);
				printf("INFO: connected %u (%d.%d.%d.%d):%u, id %u\n", event.peer->address.host, server->ip[playerID][1], server->ip[playerID][2], server->ip[playerID][3], server->ip[playerID][4], event.peer->address.port, playerID);
				break;
			case ENET_EVENT_TYPE_DISCONNECT:
				playerID				= (uint8)((size_t) event.peer->data);
				server->state[playerID] = STATE_DISCONNECTED;
				SendPlayerLeft(server, playerID);
				if (server->enableMasterConnection == 1) {
					updateMaster(server);
				}
				break;
			case ENET_EVENT_TYPE_RECEIVE:
			{
				DataStream stream = {event.packet->data, event.packet->dataLength, 0};
				playerID		  = (uint8)((size_t) event.peer->data);
				OnPacketReceived(server, playerID, &stream, event);
				enet_packet_destroy(event.packet);
				break;
			}
		}
	}
	for (uint8 playerID = 0; playerID < server->maxPlayers; ++playerID) {
		OnPlayerUpdate(server, playerID);
	}
	if (server->dirty) {
		SendWorldUpdate(server);
		server->dirty = 0;
	}
}

static void LoadMap(GameServer* server, const char* path)
{
	STATUS("loading map");

	while (server->compressedMap) {
		server->compressedMap = Pop(server->compressedMap);
	}

	FILE* file = fopen("hallway.vxl", "rb");
	if (!file) {
		perror("file not found");
		exit(EXIT_FAILURE);
	}

	fseek(file, 0, SEEK_END);
	//server->mapSize = ftell(file);
	server->mapSize = 1024*1024*64; //The size of file wasnt enough memory for it. Lets bump it up to this for now to fix building
	fseek(file, 0, SEEK_SET);

	uint8* buffer = (uint8*) malloc(server->mapSize);
	uint8* mapOut = (uint8*) malloc(server->mapSize);
	fread(buffer, server->mapSize, 1, file);
	fclose(file);
	libvxl_create(&server->map, 512, 512, 64, buffer, server->mapSize);
	STATUS("compressing map data");

	libvxl_write(&server->map, mapOut, &server->mapSize);
	server->compressedMap = CompressData(mapOut, server->mapSize, DEFAULT_COMPRESSOR_CHUNK_SIZE);
	free(buffer);
	free(mapOut);

	Queue* node = server->compressedMap;
	server->compressedSize = 0;
	while (node) {
		server->compressedSize += node->length;
		node = node->next;
	}
}

static void ServerInit(GameServer* server, uint32 connections)
{
	char team1[] = "Team A";
	char team2[] = "Team B";
	server->numPlayers = 0;
	server->maxPlayers = (connections <= 32) ? connections : 32;

	server->dirty = 0;

	server->compressedMap  = NULL;
	server->compressedSize = 0;
	server->inputFlags	 = 0;

	for (uint32 i = 0; i < server->maxPlayers; ++i) {
		server->state[i]  = STATE_DISCONNECTED;
		server->queues[i] = NULL;
		server->input[i]  = 0;
		memset(server->name[i], 0, 17);
	}

	srand(time(NULL));

	server->spawns[0].from.x = 64.f;
	server->spawns[0].from.y = 224.f;
	server->spawns[0].to.x   = 128.f;
	server->spawns[0].to.y   = 288.f;

	server->spawns[1].from.x = 382.f;
	server->spawns[1].from.y = 224.f;
	server->spawns[1].to.x   = 448.f;
	server->spawns[1].to.y   = 288.f;

	server->colorFog[0] = 0x80;
	server->colorFog[1] = 0xE8;
	server->colorFog[2] = 0xFF;

	server->colorTeamA[0] = 0xff;
	server->colorTeamA[1] = 0x00;
	server->colorTeamA[2] = 0x00;

	server->colorTeamB[0] = 0x00;
	server->colorTeamB[1] = 0xff;
	server->colorTeamB[2] = 0x00;

	memcpy(server->nameTeamA, team1, strlen(team1));
	memcpy(server->nameTeamB, team2, strlen(team2));

	server->mode = GAME_MODE_CTF;

	// Init CTF

	server->ctf.scoreTeamA = 0;
	server->ctf.scoreTeamB = 0;
	server->ctf.scoreLimit = 10;
	server->ctf.intelFlags = 0;
	// intel
	server->ctf.intelTeamA.x = 120.f;
	server->ctf.intelTeamA.y = 256.f;
	server->ctf.intelTeamA.z = 62.f;
	server->ctf.intelTeamB.x = 110.f;
	server->ctf.intelTeamB.y = 256.f;
	server->ctf.intelTeamB.z = 62.f;
	// bases
	server->ctf.baseTeamA.x = 120.f;
	server->ctf.baseTeamA.y = 250.f;
	server->ctf.baseTeamA.z = 62.f;
	server->ctf.baseTeamB.x = 110.f;
	server->ctf.baseTeamB.y = 250.f;
	server->ctf.baseTeamB.z = 62.f;

	LoadMap(server, "hallway.vxl");
}

int ConnectMaster(GameServer* server) {
    server->client = enet_host_create(NULL, 1, 1, 0, 0);

    enet_host_compress_with_range_coder(server->client);

    if (server->client == NULL) {
        fprintf(stderr, "An error occurred while trying to create an ENet client host!\n");
        return EXIT_FAILURE;
    }

    ENetAddress address;
    //ENetPeer*   peer;

    enet_address_set_host(&address, "67.205.183.163");
    address.port = 32886;

	STATUS("Connecting to master server");

    server->masterpeer = enet_host_connect(server->client, &address, 2, 31);
    if (server->masterpeer == NULL) {
        fprintf(stderr, "ERROR: failed to create client\n");
        return EXIT_FAILURE;
    }

    ENetEvent event;
    while (enet_host_service(server->client, &event, 2000) > 0) {
    				STATUS("Connection success");
				ENetPacket* packet = enet_packet_create(NULL, 61, ENET_PACKET_FLAG_RELIABLE);
				DataStream  stream = {packet->data, packet->dataLength, 0};
				WriteByte(&stream, 32);
				WriteShort(&stream, 32887);
				WriteArray(&stream, "SpadesX Server", 15);
				WriteArray(&stream, "ctf", 4);
				WriteArray(&stream, "hallway", 8);
				enet_peer_send(server->masterpeer, 0, packet);
	}
}

void ServerRun(uint16 port, uint32 connections, uint32 channels, uint32 inBandwidth, uint32 outBandwidth)
{
	STATUS("initializing ENet");

	if (enet_initialize() != 0) {
		ERROR("failed to initalize ENet");
		exit(EXIT_FAILURE);
	}
	atexit(enet_deinitialize);

	ENetAddress address;
	address.host = ENET_HOST_ANY;
	address.port = port;

	STATUS("creating server");

	GameServer server;

	server.host = enet_host_create(&address, connections, channels, inBandwidth, outBandwidth);
	if (server.host == NULL) {
		ERROR("failed to create server");
		exit(EXIT_FAILURE);
	}

	if (enet_host_compress_with_range_coder(server.host) != 0) {
		WARNING("compress with range coder failed");
	}

	STATUS("intializing server");

	ServerInit(&server, connections);

	STATUS("server started");
	server.enableMasterConnection = 0; //Change to 1 for enable or 0 to disable. Will be changed in future to read from config.
	if (server.enableMasterConnection == 1) {
		ConnectMaster(&server);
	}
	server.waitBeforeSend = time(NULL);
	while (1) {
		ServerUpdate(&server, 1);
	}

	STATUS("destroying server");
	enet_host_destroy(server.host);
}
