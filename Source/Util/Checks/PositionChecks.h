#ifndef CHECKS_H
#define CHECKS_H

#include <Server/Structs/ServerStruct.h>

uint32_t distance_in_3d(vector3f_t vector1, vector3f_t vector2);
uint32_t distance_in_2d(vector3f_t vector1, vector3f_t vector2);
uint8_t  collision_3d(vector3f_t vector1, vector3f_t vector2, uint8_t distance);
uint8_t  valid_pos_v3i(server_t* server, vector3i_t pos);
uint8_t  valid_pos_v3f(server_t* server, vector3f_t pos);
uint8_t  valid_pos_3i(server_t* server, int x, int y, int z);
uint8_t  valid_pos_3f(server_t* server, uint8_t player_id, float X, float Y, float Z);

#endif
