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

#ifndef PHYSICS_H
#define PHYSICS_H

// SpadesX
#include <Server/Structs/ServerStruct.h>
#include <Util/Types.h>

enum damage_index { BODY_TORSO, BODY_HEAD, BODY_ARMS, BODY_LEGS, BODY_MELEE };

float physics_distance_3d(float x1, float y1, float z1, float x2, float y2, float z2);
int   physics_validate_hit(vector3f_t shooter, vector3f_t orientation, vector3f_t otherPos, float tolerance);
long  physics_can_see(server_t* server, float x0, float y0, float z0, float x1, float y1, float z1);
long  physics_cast_ray(server_t* server,
                       float     x0,
                       float     y0,
                       float     z0,
                       float     x1,
                       float     y1,
                       float     z1,
                       float     length,
                       long*     x,
                       long*     y,
                       long*     z);

void physics_reorient_player(server_t* server, uint8_t player_id, vector3f_t* orientation);
int  physics_try_uncrouch(server_t* server, uint8_t player_id);
void physics_box_clip_move(server_t* server, uint8_t player_id);
long physics_move_player(server_t* server, uint8_t player_id);
int  physics_move_grenade(server_t* server, grenade_t* grenade);
void physics_set_globals(float time, float dt);

#endif
