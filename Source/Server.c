#include "Server.h"

#include "Enums.h"
#include "Types.h"

#include <Compress.h>
#include <DataStream.h>
#include <Queue.h>
#include <enet/enet.h>
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
	// respawn area
	Quad2D spawns[2];
	// dirty flag
	uint8 dirty;
	// per player
	uint8  input[32];
	uint32 inputFlags;

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
		server->queues[playerID] = server->compressedMap;
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

static void SendPlayerState(GameServer* server, uint8 playerID, uint8 otherID)
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
			SendPlayerState(server, i, playerID);
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
	//server->inputFlags |= (uint32) 1 << playerID; //Commented out for now. Causing a LOT of issues when receiving data
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

static void sendKillPacket(GameServer* server, uint8 playerID, uint8 killReason, uint8 respawnTime) {
	ENetPacket* packet = enet_packet_create(NULL, 5, ENET_PACKET_FLAG_RELIABLE);
	DataStream  stream = {packet->data, packet->dataLength, 0};
	WriteByte(&stream, PACKET_TYPE_KILL_ACTION);
	WriteByte(&stream, playerID); //Player that got killed ID
	WriteByte(&stream, playerID); //KillerID
	WriteByte(&stream, killReason); //Killing reason (1 is headshot)
	WriteByte(&stream, respawnTime); //Time before respawn happens
	enet_peer_send(server->peer[playerID], 0, packet);
	server->respawnTime[playerID] = respawnTime;
	server->startOfRespawnWait[playerID] = time(NULL);
}

static void sendMessage(ENetEvent event, DataStream* data, GameServer* server, uint8 playerID) {
	uint32 packetSize = event.packet->dataLength + 1;
	int player = ReadByte(data);
	int meantfor = ReadByte(data);
	uint32 length = DataLeft(data);
	char * message = calloc(length, sizeof (char));
	ReadArray(data, message, length);
			message[length] = '\0';
			ENetPacket* packet = enet_packet_create(NULL, packetSize, ENET_PACKET_FLAG_RELIABLE);
			DataStream  stream = {packet->data, packet->dataLength, 0};
			WriteByte(&stream, PACKET_TYPE_CHAT_MESSAGE);
			WriteByte(&stream, playerID);
			WriteByte(&stream, meantfor);
			WriteArray(&stream, message, length);
			if (message[0] == '/') {
				if (message[1] == 'k' && message[2] == 'i' && message[3] == 'l' && message[4] == 'l') {
					sendKillPacket(server, playerID, 1, 5);
					server->state[playerID] = STATE_WAITING_FOR_RESPAWN;
				}
			}
			else {
				enet_host_broadcast(server->host, 0, packet);
			}
			free(message);
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
			sendMessage(event, data, server, playerID);
			break;
		case PACKET_TYPE_BLOCK_ACTION:
		{
			uint8 player = ReadByte(data);
			uint8 actionType = ReadByte(data);
			int X = ReadInt(data);
			int Y = ReadInt(data);
			int Z = ReadInt(data);
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
			uint8 player = ReadByte(data);
			int sX = ReadInt(data);
			int sY = ReadInt(data);
			int sZ = ReadInt(data);
			int eX = ReadInt(data);
			int eY = ReadInt(data);
			int eZ = ReadInt(data);
			ENetPacket* packet = enet_packet_create(NULL, 26, ENET_PACKET_FLAG_RELIABLE);
			DataStream  stream = {packet->data, packet->dataLength, 0};
			WriteByte(&stream, PACKET_TYPE_BLOCK_LINE);
			WriteByte(&stream, player);
			WriteInt(&stream, sX);
			WriteInt(&stream, sY);
			WriteInt(&stream, sZ);
			WriteInt(&stream, eX);
			WriteInt(&stream, eY);
			WriteInt(&stream, eZ);
			enet_host_broadcast(server->host, 0, packet);
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
			ENetPacket* packet = enet_packet_create(NULL, 5, ENET_PACKET_FLAG_RELIABLE);
			DataStream  stream = {packet->data, packet->dataLength, 0};
			WriteByte(&stream, PACKET_TYPE_SET_COLOR);
			WriteByte(&stream, player);
			WriteByte(&stream, B);
			WriteByte(&stream, R);
			WriteByte(&stream, G);
			for (uint8 i = 0; i < 32; ++i) {
				if (playerID != i && server->state[i] != STATE_DISCONNECTED) {
					enet_peer_send(server->peer[i], 0, packet);
				}
			}
			break;
		}
		default:
			printf("unhandled input, id %u, code %u\n", playerID, type);
			break;
	}
}

static void SendInputData(GameServer* server, uint8 playerID)
{
	for (uint8 i = 0; i < 32; ++i) {
		if (playerID != i && server->state[i] != STATE_DISCONNECTED) {
			ENetPacket* packet = enet_packet_create(NULL, 1 + (32 * 24), ENET_PACKET_FLAG_UNSEQUENCED);
			DataStream  stream = {packet->data, packet->dataLength, 0};
			WriteByte(&stream, PACKET_TYPE_INPUT_DATA);
			WriteByte(&stream, server->input[playerID]);
			enet_host_broadcast(server->host, 0, packet);
		}
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
			if (server->inputFlags & ((uint32) 1 << playerID)) {
				SendInputData(server, playerID);
				server->inputFlags &= ~((uint32) 1 << playerID);
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
				printf("INFO: connected %u:%u, id %u\n", event.peer->address.host, event.peer->address.port, playerID);
				break;
			case ENET_EVENT_TYPE_DISCONNECT:
				playerID				= (uint8)((size_t) event.peer->data);
				server->state[playerID] = STATE_DISCONNECTED;
				SendPlayerLeft(server, playerID);
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
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);

	uint8* buffer = (uint8*) malloc(size);
	fread(buffer, size, 1, file);
	fclose(file);

	STATUS("compressing map data");

	server->compressedMap = CompressData(buffer, size, DEFAULT_COMPRESSOR_CHUNK_SIZE);
	free(buffer);

	Queue* node			= server->compressedMap;
	server->compressedSize = 0;
	while (node) {
		server->compressedSize += node->length;
		node = node->next;
	}
}

static void ServerInit(GameServer* server, uint32 connections)
{
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

	server->colorFog[0] = 0xff;
	server->colorFog[1] = 0xff;
	server->colorFog[2] = 0xff;

	server->colorTeamA[0] = 0xff;
	server->colorTeamA[1] = 0x00;
	server->colorTeamA[2] = 0x00;

	server->colorTeamB[0] = 0x00;
	server->colorTeamB[1] = 0xff;
	server->colorTeamB[2] = 0x00;

	memcpy(server->nameTeamA, "Team A", sizeof("Team A"));
	memcpy(server->nameTeamB, "Team B", sizeof("Team B"));

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

	if (InitCompressor(5) != 0) {
		WARNING("failed to initialize compressor");
	}

	ServerInit(&server, connections);

	STATUS("server started");

	while (1) {
		ServerUpdate(&server, 1);
	}

	STATUS("destroying server");
	CloseCompressor();
	enet_host_destroy(server.host);
}
