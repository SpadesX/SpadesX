#include <Server/Structs/ServerStruct.h>
#include <math.h>

uint32_t distance_in_3d(vector3f_t vector1, vector3f_t vector2)
{
    uint32_t distance = 0;
    distance          = sqrt(fabs(pow(vector1.x - vector2.x, 2)) + fabs(pow(vector1.y - vector2.y, 2)) +
                    fabs(pow(vector1.z - vector2.z, 2)));
    return distance;
}

uint32_t distance_in_2d(vector3f_t vector1, vector3f_t vector2)
{
    uint32_t distance = 0;
    distance          = sqrt(fabs(pow(vector1.x - vector2.x, 2)) + fabs(pow(vector1.y - vector2.y, 2)));
    return distance;
}

uint8_t collision_3d(vector3f_t vector1, vector3f_t vector2, uint8_t distance)
{
    if (fabs(vector1.x - vector2.x) < distance && fabs(vector1.y - vector2.y) < distance &&
        fabs(vector1.x - vector2.x) < distance)
    {
        return 0;
    } else {
        return 1;
    }
}

uint8_t valid_pos_v3i(server_t* server, vector3i_t pos)
{
    if (pos.x < server->s_map.map.size_x && pos.x >= 0 && pos.y < server->s_map.map.size_y && pos.y >= 0 &&
        pos.z < server->s_map.map.size_z && pos.z >= 0)
    {
        return 1;
    } else {
        return 0;
    }
}

uint8_t valid_pos_v3f(server_t* server, vector3f_t pos)
{
    if (pos.x < server->s_map.map.size_x && pos.x >= 0 && pos.y < server->s_map.map.size_y && pos.y >= 0 &&
        pos.z < server->s_map.map.size_z && pos.z >= 0)
    {
        return 1;
    } else {
        return 0;
    }
}
uint8_t valid_pos_3i(server_t* server, int x, int y, int z)
{
    if (x < server->s_map.map.size_x && x >= 0 && y < server->s_map.map.size_y && y >= 0 &&
        z < server->s_map.map.size_z && z >= 0)
    {
        return 1;
    } else {
        return 0;
    }
}

uint8_t valid_pos_3f(server_t* server, player_t* player, float X, float Y, float Z)
{
    int x = (int) X;
    int y = (int) Y;
    int z = 0;
    if (Z < 0.f) {
        z = (int) Z + 1;
    } else {
        z = (int) Z + 2;
    }

    if (x < server->s_map.map.size_x && x >= 0 && y < server->s_map.map.size_y && y >= 0 &&
        (z < server->s_map.map.size_z || (z == 64 && player->crouching)) &&
        (z >= 0 || ((z == -1) || (z == -2 && player->jumping))))
    {
        if ((!mapvxl_is_solid(&server->s_map.map, x, y, z) ||
             (z == 63 || z == -1 || (z == -2 && player->jumping) ||
              (z == 64 && player->crouching) || player->jumping) ||
             (mapvxl_is_solid(&server->s_map.map, x, y, z) && player->crouching)) &&
            (!mapvxl_is_solid(&server->s_map.map, x, y, z - 1) ||
             ((z <= 1 && z > -2) || (z == -2 && player->jumping)) ||
             (z - 1 == 63 && player->crouching)) &&
            (!mapvxl_is_solid(&server->s_map.map, x, y, z - 2) ||
             ((z <= 2 && z > -2) || (z == -2 && player->jumping))))
        /* Dont even think about this
        This is what happens when map doesnt account for full height of
        freaking player and I have to check for out of bounds checks on map...
        */
        {
            return 1;
        }
    }
    return 0;
}
