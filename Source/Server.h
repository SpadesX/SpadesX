#ifndef SERVER_H
#define SERVER_H

#include <Types.h>
#include <Enums.h>
#include <Queue.h>
#include <Line.h>
#include <enet/enet.h>
#include <libvxl/libvxl.h>

#ifndef DEFAULT_SERVER_PORT
    #define DEFAULT_SERVER_PORT 32887
#endif

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
	
	uint8 allowTK;
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
	uint8		alive[32];
} GameServer;

void ServerRun(uint16 port, uint32 connections, uint32 channels, uint32 inBandwidth, uint32 outBandwidth);

#endif /* SERVER_H */
