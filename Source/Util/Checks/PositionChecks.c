#include <Server/Structs/ServerStruct.h>
#include <assert.h>
#include <math.h>

inline uint32_t distance_in_3d(vector3f_t vector1, vector3f_t vector2)
{
    float distance = sqrt(fabs(pow(vector1.x - vector2.x, 2)) + fabs(pow(vector1.y - vector2.y, 2)) +
                          fabs(pow(vector1.z - vector2.z, 2)));

    assert(isnan(distance) == 0);
    return distance;
}

inline uint32_t distance_in_2d(vector3f_t vector1, vector3f_t vector2)
{
    float distance = sqrt(fabs(pow(vector1.x - vector2.x, 2)) + fabs(pow(vector1.y - vector2.y, 2)));

    assert(isnan(distance) == 0);
    return distance;
}

inline uint8_t collision_3d(vector3f_t vector1, vector3f_t vector2, uint8_t distance)
{
    return !(fabs(vector1.x - vector2.x) < distance && fabs(vector1.y - vector2.y) < distance &&
             fabs(vector1.z - vector2.z) < distance);
}

inline uint8_t valid_pos_v3i(server_t* server, vector3i_t pos)
{
    return pos.x < server->s_map.map.size_x && pos.x >= 0 && pos.y < server->s_map.map.size_y && pos.y >= 0 &&
           pos.z < server->s_map.map.size_z && pos.z >= 0;
}

inline uint8_t valid_pos_v3f(server_t* server, vector3f_t pos)
{
    return pos.x < server->s_map.map.size_x && pos.x >= 0 && pos.y < server->s_map.map.size_y && pos.y >= 0 &&
           pos.z < server->s_map.map.size_z && pos.z >= 0;
}

inline uint8_t valid_pos_v3f_below_z(server_t* server, vector3f_t pos)
{
    return pos.x < server->s_map.map.size_x && pos.x >= 0 && pos.y < server->s_map.map.size_y && pos.y >= 0 &&
           pos.z < server->s_map.map.size_z && pos.z >= -4;
}

inline uint8_t valid_pos_3i(server_t* server, int x, int y, int z)
{
    return x < server->s_map.map.size_x && x >= 0 && y < server->s_map.map.size_y && y >= 0 &&
           z < server->s_map.map.size_z && z >= 0;
}

inline uint8_t valid_pos_3f(server_t* server, float x, float y, float z)
{
    return x < server->s_map.map.size_x && x >= 0 && y < server->s_map.map.size_y && y >= 0 &&
           z < server->s_map.map.size_z && z >= 0;
}

uint8_t valid_player_pos(server_t* server, player_t* player, float X, float Y, float Z)
{
    uint8_t solid_z = Z < 0 ? 0 : mapvxl_is_solid(&server->s_map.map, X, Y, Z);
    uint8_t solid_zp1 = Z + 1 < 0 ? 0 : mapvxl_is_solid(&server->s_map.map, X, Y, Z + 1);
    uint8_t solid_zp2 = Z + 2 < 0 ? 0 : mapvxl_is_solid(&server->s_map.map, X, Y, Z + 2);

    /*Player walking on top of the roof has Z value of -2 and when jumping this can go to -4.
      Player crouching will have their feet (solid_zp2) inside an solid block.
      And also water acts as solid block.
      And lastly player crouching inside water will have their middle of the body
      inside water thus its solid (Feet wont be inside a solid block)*/
    if ((X < server->s_map.map.size_x && X >= 0) && (Y < server->s_map.map.size_y && Y >= 0) &&
        (Z <= server->s_map.map.size_z && Z >= -4) &&
        (!solid_zp2 || Z == server->s_map.map.size_z - 3 || player->crouching) &&
        (!solid_zp1 || (Z == server->s_map.map.size_z - 2 && player->crouching)) &&
        (!solid_z))
    {
        return 1;
    }
    return 0;
}
