// Copyright DarkNeutrino 2021
#ifndef STRUCTS_H
#define STRUCTS_H

#include "enet/protocol.h"

#include <Enums.h>
#include <Queue.h>
#include <Types.h>
#include <enet/enet.h>
#include <libmapvxl/libmapvxl.h>
#include <time.h>

#define VERSION        "0.0.50"
#define MAX_MAP_COUNT  64 // Change this if you have more then 64 maps. Tho ask yourself first WHY.

#define MAP_MAX_X MAP_X_MAX
#define MAP_MAX_Y MAP_Y_MAX
#define MAP_MAX_Z MAP_Z_MAX

//#define DEBUG

typedef union
{
    uint8  ip[4];
    uint32 ip32;
} IPUnion;

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
    uint64   timeSinceSent;
    uint64   timeNow;
} Grenade;

typedef struct
{ // Seriously what the F. Thank voxlap motion for this mess.
    Vector3f position;
    Vector3f prevLegitPos;
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
    Vector3i          resultLine[50];
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
    uint8     playerIntelTeam[2];
    uint8     intelHeld[2];
} GameModeVars;

typedef struct
{
    uint8 countOfUsers;
    uint8 teamUserCount[2];
    //
    uint8 numPlayers;
    uint8 maxPlayers;
    //
    Color3i      colorFog;
    Color3i      colorTeam[2];
    char         nameTeam[2][11];
    GameMode     currentGameMode;
    GameModeVars gameMode;
    // respawn area
    Quad3D spawns[2];
    uint32 inputFlags;
} Protocol;

typedef struct
{
    uint64 timeSinceLastWU;
    uint64 sinceLastBaseEnter;
    uint64 sinceLastBaseEnterRestock;
    uint64 startOfRespawnWait;
    uint64 sinceLastShot;
    uint64 sinceLastBlockDest;
    uint64 sinceLastBlockPlac;
    uint64 sinceLast3BlockDest;
    uint64 sinceLastGrenadeThrown;
    uint64 sinceLastWeaponInput;
    uint64 sinceReloadStart;
    uint64 sinceLastPrimaryWeaponInput;
} Timers;

typedef struct
{
    unsigned long long updateTime;
    unsigned long long lastUpdateTime;
    unsigned long long timeSinceStart;
    float              timeSinceStartSimulated;
} GlobalTimers;

typedef struct
{
    Queue*               queues;
    State                state;
    Weapon               weapon;
    Team                 team;
    Tool                 item;
    PermLevel            roleList[5]; // Change me based on the number of access levels you require
    Timers               timers;
    WeaponDefaultClip    defaultClip;
    WeaponDefaultReserve defaultReserve;
    uint32               kills;
    uint32               deaths;
    Color3i              color;
    char                 name[17];
    char                 client;
    int                  version_minor;
    int                  version_major;
    int                  version_revision;
    char                 os_info[255];
    ENetPeer*            peer;
    uint8                respawnTime;
    Color4i              toolColor;
    int                  HP;
    uint8                grenades;
    uint8                blocks;
    uint8                weaponReserve;
    short                weaponClip;
    Vector3i             resultLine[50];
    IPUnion              ipUnion;
    uint8                alive;
    uint8                input;
    uint8                allowKilling;
    uint8                allowTeamKilling;
    uint16               ups;
    uint8                muted;
    uint8                canBuild;
    Grenade              grenade[3];
    uint8                toldToMaster;
    uint8                hasIntel;
    uint8                isManager;
    uint8                isAdmin;
    uint8                isMod;
    uint8                isGuard;
    uint8                isTrusted;
    uint8                isInvisible;
    uint8                welcomeSent;
    uint8                toRefill;
    uint8                reloading;
    uint8                firstBullet;

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
    uint64    timeSinceLastSend;
} Master;

typedef struct
{
    ENetHost*      host;
    Player         player[32];
    Protocol       protocol;
    Master         master;
    uint16         port;
    Map            map;
    GlobalTimers   globalTimers;
    uint8          globalAK;
    uint8          globalAB;
    const char*    managerPasswd;
    const char*    adminPasswd;
    const char*    modPasswd;
    const char*    guardPasswd;
    const char*    trustedPasswd;
    char           serverName[31];
    char           mapName[20];
    char           gamemodeName[7];
    volatile uint8 running; // volatile keyword is required to have an access to this variable in any thread
} Server;

#endif
