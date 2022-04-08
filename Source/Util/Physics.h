/*
    Copyright (c) Mathias Kaerlev 2011-2012.

    This file is part of pyspades.

    pyspades is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    pyspades is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with pyspades.  If not, see <http://www.gnu.org/licenses/>.

*/

#define CHUNK                1023 // zlib buffer size
#define VSIDSQM              (VSIDSQ - 1)
#define MAXSCANDIST          128
#define MAXSCANSQ            (MAXSCANDIST * MAXSCANDIST)
#define VOXSIZ               (VSIDSQ * MAXZDIM)
#define SCPITCH              128
#define SQRT                 0.70710678f
#define MINERANGE            3
#define MAXZDIM              MAP_MAX_Z // Maximum .VXL dimensions in z direction (height)
#define MAXZDIMM             (MAXZDIM - 1)
#define MAXZDIMMM            (MAXZDIM - 2)
#define PORT                 32887
#define GRID_SIZE            64
#define FALL_SLOW_DOWN       0.24f
#define FALL_DAMAGE_VELOCITY 0.58f
#define FALL_DAMAGE_SCALAR   4096
#define MINERANGE            3
#define WEAPON_PRIMARY       1
#define PI                   3.141592653589793f
#define VSID                 MAP_MAX_X // maximum .VXL dimensions in both x & y direction
#define VSIDM                (VSID - 1)
#define VSIDSQ               (VSID * VSID)
#define CUBE_ARRAY_LENGTH    64

#ifndef PHYSICS_H
#define PHYSICS_H

// SpadesX
#include "../Structs.h"
#include "Types.h"

enum damage_index { BODY_TORSO, BODY_HEAD, BODY_ARMS, BODY_LEGS, BODY_MELEE };

typedef struct
{
    float x;
    float y;
    float z;
} Vector;

float distance3d(float x1, float y1, float z1, float x2, float y2, float z2);

int validateHit(Vector3f shooter, Vector3f orientation, Vector3f otherPos, float tolerance);

long canSee(Server* server, float x0, float y0, float z0, float x1, float y1, float z1);

long castRay(Server* server,
             float   x0,
             float   y0,
             float   z0,
             float   x1,
             float   y1,
             float   z1,
             float   length,
             long*   x,
             long*   y,
             long*   z);

void reorientPlayer(Server* server, uint8 playerID, Vector3f* orientation);

int tryUncrouch(Server* server, uint8 playerID);

void boxclipmove(Server* server, uint8 playerID);

long movePlayer(Server* server, uint8 playerID);

int moveGrenade(Server* server, Grenade* grenade);

void setPhysicsGlobals(float time, float dt);

#endif
