#ifndef PLAYER_H
#define PLAYER_H

#include "Enums.h"
#include "Queue.h"
#include "Types.h"

#include <enet/enet.h>

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
    uint64    toolColor;
    uint8     weaponReserve;
    short     weaponClip;
    vec3i     resultLine[50];
    uint8     ip[4];
    uint8     alive;
    uint8	  input;
} Player;

#endif
