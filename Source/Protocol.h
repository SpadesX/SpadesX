#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "Enums.h"
#include "Queue.h"
#include "Types.h"

#include <enet/enet.h>

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
    time_t    waitBeforeSend;
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
    // compressed map
    Queue* compressedMap;
    uint32 compressedSize;
    uint32 mapSize;
    // respawn area
    Quad2D spawns[2];
    uint32    inputFlags;
} Protocol;

#endif
