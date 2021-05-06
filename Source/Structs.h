#ifndef STRUCTS_H
#define STRUCTS_H

#include <bits/types/struct_timeval.h>
#include <enet/enet.h>
#include <libvxl/libvxl.h>
#include <time.h>

#include "Enums.h"
#include "Queue.h"
#include "Types.h"
#include "Util/Line.h"

typedef struct {
    // compressed map
    Queue* compressedMap;
    uint32 compressedSize;
    vec3i resultLine[50];
    long unsigned int mapSize;
    struct libvxl_map map;
} Map;

typedef struct
{
    uint8     scoreTeamA;
    uint8     scoreTeamB;
    uint8     scoreLimit;
    IntelFlag intelFlags;
    Vector3f  intelTeamA;
    Vector3f  intelTeamB;
    Vector3f  baseTeamA;
    Vector3f  baseTeamB;
    uint8     playerIntelTeamA; // player ID if intel flags & 1 != 0
    uint8     playerIntelTeamB; // player ID if intel flags & 2 != 0
} ModeCTF;

typedef struct {
    uint8     countOfUsers;
    //
    uint8 numPlayers;
    uint8 maxPlayers;
    //
    Color3i  colorFog;
    Color3i  colorTeamA;
    Color3i  colorTeamB;
    char     nameTeamA[11];
    char     nameTeamB[11];
    GameMode mode;
    // mode
    ModeCTF ctf;
    // respawn area
    Quad2D spawns[2];
    uint32    inputFlags;
} Protocol;

typedef struct {
	Queue*    queues;
    State     state;
    Weapon    weapon;
    Team      team;
    Tool      item;
    uint32    kills;
    Color3i   color;
    Vector3f  pos;
    Vector3f  rot;
    char      name[17];
    ENetPeer* peer;
    uint8     respawnTime;
    uint32    startOfRespawnWait;
    uint8     HP;
    Color4i   toolColor;
    uint8     weaponReserve;
    short     weaponClip;
    vec3i     resultLine[50];
    uint8     ip[4];
    uint8     alive;
    uint8	  input;
    uint8	  allowTK;
    unsigned long long timeSinceLastWU;
    uint16    ups;
} Player;

typedef struct {
	// Master
    ENetHost* client;
    ENetPeer* peer;
    uint8     enableMasterConnection;
    time_t    timeSinceLastSend;
} Master;

typedef struct {
	ENetHost* host;
	Player player[32];
	Protocol protocol;
	Master master;
    Map map;
    uint8 dirty;
} Server;

#endif
