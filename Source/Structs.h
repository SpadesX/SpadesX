// Copyright DarkNeutrino 2021
#ifndef STRUCTS_H
#define STRUCTS_H

#include "Enums.h"
#include "enet/protocol.h"

#include <Line.h>
#include <Queue.h>
#include <Types.h>
#include <bits/types/struct_timeval.h>
#include <enet/enet.h>
#include <libmapvxl/libmapvxl.h>
#include <time.h>

#define NANO_IN_SECOND 1000000000
#define NANO_IN_MILLI 1000000
#define VERSION "0.0.2"
#define MAX_MAP_COUNT 64 //Change this if you have more then 64 maps. Tho ask yourself first WHY.

typedef struct
{
    char         accessLevel[32];
    const char** accessPassword;
    uint8*       access;
} PermLevel;

typedef struct
{
    uint8    sent;
    float    fuse;
    uint8    exploded;
    Vector3f position;
    Vector3f velocity;
    time_t   timeSinceSent;
    time_t   timeNow;
} Grenade;

typedef struct
{ // Seriously what the F. Thank voxlap motion for this mess.
    Vector3f position;
    Vector3f eyePos;
    Vector3f velocity;
    Vector3f strafeOrientation;
    Vector3f heightOrientation;
    Vector3f forwardOrientation;
    Vector3f previousOrientation;
} Movement;

typedef struct
{ // Yet again thanks voxlap.
    Vector3f forward;
    Vector3f strafe;
    Vector3f height;
} Orientation;

typedef struct
{
    uint8 mapCount;
    uint8 mapIndex;
    // compressed map
    Queue*            compressedMap;
    uint32            compressedSize;
    vec3i             resultLine[50];
    long unsigned int mapSize;
    MapVxl            map;
    char              mapArray[MAX_MAP_COUNT][64];
} Map;

typedef struct
{
    uint8     score[2];
    uint8     scoreLimit;
    IntelFlag intelFlags;
    Vector3f  intel[2];
    Vector3f  base[2];
    uint8     playerIntelTeamA; // player ID if intel flags & 1 != 0
    uint8     playerIntelTeamB; // player ID if intel flags & 2 != 0
    uint8     intelHeld[2];
} ModeCTF;

typedef struct
{
    uint8 countOfUsers;
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
    Quad3D spawns[2];
    uint32 inputFlags;
} Protocol;

typedef struct
{
    time_t timeSinceLastWU;
    time_t sinceLastBaseEnter;
    time_t sinceLastBaseEnterRestock;
    time_t startOfRespawnWait;
    time_t sinceLastShot;
    time_t sinceLastBlockDest;
    time_t sinceLastBlockPlac;
    time_t sinceLast3BlockDest;
    time_t sinceLastGrenadeThrown;
} Timers;

typedef struct
{
    Queue*    queues;
    State     state;
    Weapon    weapon;
    Team      team;
    Tool      item;
    PermLevel roleList[5]; // Change me based on the number of access levels you require
    Timers    timers;
    uint32    kills;
    uint32    deaths;
    Color3i   color;
    char      name[17];
    ENetPeer* peer;
    uint8     respawnTime;
    Color4i   toolColor;
    int       HP;
    uint8     grenades;
    uint8     blocks;
    uint8     weaponReserve;
    short     weaponClip;
    vec3i     resultLine[50];
    uint8     ip[4];
    uint8     alive;
    uint8     input;
    uint8     allowKilling;
    uint8     allowTeamKilling;
    uint16    ups;
    uint8     muted;
    uint8     canBuild;
    Grenade   grenade[3];
    uint8     toldToMaster;
    uint8     hasIntel;
    uint8     isManager;
    uint8     isAdmin;
    uint8     isMod;
    uint8     isGuard;
    uint8     isTrusted;
    uint8     isInvisible;

    uint8    movForward;
    uint8    movBackwards;
    uint8    movLeft;
    uint8    movRight;
    uint8    jumping;
    uint8    crouching;
    uint8    sneaking;
    uint8    sprinting;
    uint8    primary_fire;
    uint8    secondary_fire;
    Vector3f locAtClick;

    // Bellow this point is stuff used for calculating movement.
    Movement movement;
    uint8    airborne;
    uint8    wade;
    float    lastclimb;
} Player;

typedef struct
{
    // Master
    ENetHost* client;
    ENetPeer* peer;
    ENetEvent event;
    uint8     enableMasterConnection;
    time_t    timeSinceLastSend;
} Master;

typedef struct
{
    ENetHost*   host;
    Player      player[32];
    Protocol    protocol;
    Master      master;
    Map         map;
    uint8       globalAK;
    uint8       globalAB;
    const char* managerPasswd;
    const char* adminPasswd;
    const char* modPasswd;
    const char* guardPasswd;
    const char* trustedPasswd;
    char        serverName[31];
    char        mapName[20];
    char        gamemodeName[7];
    uint8       running;
} Server;

#endif
