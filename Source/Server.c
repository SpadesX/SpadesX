//Copyright DarkNeutrino 2021
#include "Server.h"

#include "Structs.h"
#include "Enums.h"
#include "Types.h"
#include "Line.h"
#include "Conversion.h"
#include "Master.h"
#include "Map.h"
#include "Player.h"
#include "Protocol.h"
#include "Packets.h"
#include "PacketReceive.h"

#include <Compress.h>
#include <DataStream.h>
#include <Queue.h>
#include <enet/enet.h>
#include <libvxl/libvxl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

	struct arg_struct {
    Server* server;
    uint8 playerID;
	};

static unsigned long long get_nanos(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (unsigned long long)ts.tv_sec * 1000000000L + ts.tv_nsec;
}

static void ServerInit(Server* server, uint32 connections, char* map)
{
	char team1[] = "Team A";
	char team2[] = "Team B";
	server->protocol.numPlayers = 0;
	server->protocol.maxPlayers = (connections <= 32) ? connections : 32;

	server->map.compressedMap  = NULL;
	server->map.compressedSize = 0;
	server->protocol.inputFlags = 0;
	
	for (uint8 i = 0; i < 11; i++) {
		server->protocol.nameTeamA[i] = ' ';
		server->protocol.nameTeamB[i] = ' ';
	}

	for (uint32 i = 0; i < server->protocol.maxPlayers; ++i) {
		server->player[i].state  = STATE_DISCONNECTED;
		server->player[i].queues = NULL;
		server->player[i].ups = 60;
		server->player[i].timeSinceLastWU = get_nanos();
		server->player[i].input  = 0;
		memset(server->player[i].name, 0, 17);
	}

	srand(time(NULL));

	server->protocol.spawns[0].from.x = 64.f;
	server->protocol.spawns[0].from.y = 224.f;
	server->protocol.spawns[0].to.x   = 128.f;
	server->protocol.spawns[0].to.y   = 288.f;

	server->protocol.spawns[1].from.x = 382.f;
	server->protocol.spawns[1].from.y = 224.f;
	server->protocol.spawns[1].to.x   = 448.f;
	server->protocol.spawns[1].to.y   = 288.f;

	server->protocol.colorFog[0] = 0x80;
	server->protocol.colorFog[1] = 0xE8;
	server->protocol.colorFog[2] = 0xFF;

	server->protocol.colorTeamA[0] = 0xff;
	server->protocol.colorTeamA[1] = 0x00;
	server->protocol.colorTeamA[2] = 0x00;

	server->protocol.colorTeamB[0] = 0x00;
	server->protocol.colorTeamB[1] = 0xff;
	server->protocol.colorTeamB[2] = 0x00;

	memcpy(server->protocol.nameTeamA, team1, strlen(team1));
	memcpy(server->protocol.nameTeamB, team2, strlen(team2));

	server->protocol.mode = GAME_MODE_CTF;

	// Init CTF

	server->protocol.ctf.scoreTeamA = 0;
	server->protocol.ctf.scoreTeamB = 0;
	server->protocol.ctf.scoreLimit = 10;
	server->protocol.ctf.intelFlags = 0;
	// intel
	server->protocol.ctf.intelTeamA.x = 120.f;
	server->protocol.ctf.intelTeamA.y = 256.f;
	server->protocol.ctf.intelTeamA.z = 62.f;
	server->protocol.ctf.intelTeamB.x = 110.f;
	server->protocol.ctf.intelTeamB.y = 256.f;
	server->protocol.ctf.intelTeamB.z = 62.f;
	// bases
	server->protocol.ctf.baseTeamA.x = 120.f;
	server->protocol.ctf.baseTeamA.y = 250.f;
	server->protocol.ctf.baseTeamA.z = 62.f;
	server->protocol.ctf.baseTeamB.x = 110.f;
	server->protocol.ctf.baseTeamB.y = 250.f;
	server->protocol.ctf.baseTeamB.z = 62.f;

	LoadMap(server, map);
}

static void SendJoiningData(Server* server, uint8 playerID)
{
	STATUS("sending state");
	SendStateData(server, playerID);
	for (uint8 i = 0; i < server->protocol.maxPlayers; ++i) {
		if (i != playerID && server->player[i].state != STATE_DISCONNECTED) {
			SendPlayerState(server, playerID, i);
		}
	}
}

static uint8 OnConnect(Server* server)
{
	if (server->protocol.numPlayers == server->protocol.maxPlayers) {
		return 0xFF;
	}
	uint8 playerID;
	for (playerID = 0; playerID < server->protocol.maxPlayers; ++playerID) {
		if (server->player[playerID].state == STATE_DISCONNECTED) {
			server->player[playerID].state = STATE_STARTING_MAP;
			break;
		}
	}
	server->protocol.numPlayers++;
	return playerID;
}

static void OnPlayerUpdate(Server* server, uint8 playerID)
{
	switch (server->player[playerID].state) {
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
			server->player[playerID].HP = 100;
			server->player[playerID].alive = 1;
			SetPlayerRespawnPoint(server, playerID);
			SendRespawn(server, playerID);
			break;
		case STATE_WAITING_FOR_RESPAWN:
		{
			if (time(NULL) - server->player[playerID].startOfRespawnWait >= server->player[playerID].respawnTime) {
				server->player[playerID].state = STATE_SPAWNING;
			}
			break;
		}
		case STATE_READY:
			// send data
			if (server->master.enableMasterConnection == 1) {
				updateMaster(server);
			}
			break;
		default:
			// disconnected
			break;
	}
}

static void ServerUpdate(Server* server, int timeout)
{
	ENetEvent event;
	if (server->master.enableMasterConnection == 1) {
		if (time(NULL) - server->master.timeSinceLastSend >= 1) {
		ENetEvent eventMaster;
		while (enet_host_service(server->master.client, &eventMaster, 1) > 0) {
		//Quite literally here to keep the connection alive
		}
		server->master.timeSinceLastSend = time(NULL);
		}
	}
	while (enet_host_service(server->host, &event, timeout) > 0) {
		uint8 playerID;
		switch (event.type) {
			case ENET_EVENT_TYPE_NONE:
				STATUS("Event of type none received. Ignoring");
			break;
			case ENET_EVENT_TYPE_CONNECT:
				if (event.data != VERSION_0_75) {
					enet_peer_disconnect(event.peer, REASON_WRONG_PROTOCOL_VERSION);
					break;
				}
				// check peer
				// ...
				// find next free ID
				playerID = OnConnect(server);
				if (playerID == 0xFF) {
					enet_peer_disconnect(event.peer, REASON_SERVER_FULL);
					break;
				}
				server->player[playerID].peer = event.peer;
				event.peer->data	   = (void*) ((size_t) playerID);
				server->player[playerID].HP = 100;
				uint32ToUint8(server, event, playerID);
				printf("INFO: connected %u (%d.%d.%d.%d):%u, id %u\n", event.peer->address.host, server->player[playerID].ip[0], server->player[playerID].ip[1], server->player[playerID].ip[2], server->player[playerID].ip[3], event.peer->address.port, playerID);
				break;
			case ENET_EVENT_TYPE_DISCONNECT:
				playerID				= (uint8)((size_t) event.peer->data);
				server->player[playerID].state = STATE_DISCONNECTED;
				SendPlayerLeft(server, playerID);
				server->player[playerID].ups = 10;
				if (server->master.enableMasterConnection == 1) {
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

	for (uint8 playerID = 0; playerID < server->protocol.maxPlayers; playerID++) {
		OnPlayerUpdate(server, playerID);
		if (server->player[playerID].state == STATE_READY) {
			unsigned long long time = get_nanos();
			if (time - server->player[playerID].timeSinceLastWU >= (1000000000/server->player[playerID].ups)) {
				SendWorldUpdate(server, playerID);
				server->player[playerID].timeSinceLastWU = get_nanos();
			}
		}
	}
}

void StartServer(uint16 port, uint32 connections, uint32 channels, uint32 inBandwidth, uint32 outBandwidth, uint8 master, char* map)
{
	STATUS("Welcome to SpadesX server");
	STATUS("Initializing ENet");

	if (enet_initialize() != 0) {
		ERROR("Failed to initalize ENet");
		exit(EXIT_FAILURE);
	}
	atexit(enet_deinitialize);

	ENetAddress address;
	address.host = ENET_HOST_ANY;
	address.port = port;

	printf("Creating server at port %d\n", port);

	Server server;

	server.host = enet_host_create(&address, connections, channels, inBandwidth, outBandwidth);
	if (server.host == NULL) {
		ERROR("Failed to create server");
		exit(EXIT_FAILURE);
	}

	if (enet_host_compress_with_range_coder(server.host) != 0) {
		WARNING("Compress with range coder failed");
	}

	STATUS("Intializing server");

	ServerInit(&server, connections, map);

	STATUS("Server started");
	server.master.enableMasterConnection = master;
	if (server.master.enableMasterConnection == 1) {
		ConnectMaster(&server, port);
	}
	server.master.timeSinceLastSend = time(NULL);
	while (1) {
		ServerUpdate(&server, 0);
		usleep(1); //otherwise we would run at 100% cpu usage all the time. Not exactly what we want :P
	}

	STATUS("Destroying server");
	enet_host_destroy(server.host);
}
