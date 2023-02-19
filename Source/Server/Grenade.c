#include "Util/Types.h"
#include <Server/Gamemodes/Gamemodes.h>
#include <Server/IntelTent.h>
#include <Server/Nodes.h>
#include <Server/Packets/Packets.h>
#include <Server/Structs/ServerStruct.h>
#include <Util/Checks/PositionChecks.h>
#include <Util/Enums.h>
#include <Util/Log.h>
#include <Util/Nanos.h>
#include <Util/Physics.h>
#include <Util/Uthash.h>
#include <Util/Utlist.h>
#include <math.h>

vector3i_t* getGrenadeNeighbors(vector3i_t pos)
{
    static vector3i_t neighArray[54];

    // Lookup table for neighbor offsets
    static const int offsets[54][3] = {
        {0, 0, -2}, {1, 0, -2}, {-1, 0, -2}, {0, 1, -2}, {0, -1, -2},
        {1, 1, -2}, {1, -1, -2}, {-1, 1, -2}, {-1, -1, -2},
        {0, -2, 0}, {1, -2, 0}, {-1, -2, 0}, {0, -2, 1}, {0, -2, -1},
        {1, -2, 1}, {1, -2, -1}, {-1, -2, 1}, {-1, -2, -1},
        {0, 2, 0}, {1, 2, 0}, {-1, 2, 0}, {0, 2, 1}, {0, 2, -1},
        {1, 2, 1}, {1, 2, -1}, {-1, 2, 1}, {-1, 2, -1},
        {-2, 0, 0}, {-2, 1, 0}, {-2, -1, 0}, {-2, 0, 1}, {-2, 0, -1},
        {-2, 1, 1}, {-2, 1, -1}, {-2, -1, 1}, {-2, -1, -1},
        {0, 0, 2}, {1, 0, 2}, {-1, 0, 2}, {0, 1, 2}, {0, -1, 2},
        {1, 1, 2}, {1, -1, 2}, {-1, 1, 2}, {-1, -1, 2}
    };

    for (int i = 0; i < 54; i++) {
        neighArray[i].x = pos.x + offsets[i][0];
        neighArray[i].y = pos.y + offsets[i][1];
        neighArray[i].z = pos.z + offsets[i][2];
    }

    return neighArray;
}

uint8_t get_grenade_damage(server_t* server, player_t* damaged_player, grenade_t* grenade)
{
    double     diffX      = fabs(damaged_player->movement.position.x - grenade->position.x);
    double     diffY      = fabs(damaged_player->movement.position.y - grenade->position.y);
    double     diffZ      = fabs(damaged_player->movement.position.z - grenade->position.z);
    vector3f_t playerPos  = damaged_player->movement.position;
    vector3f_t grenadePos = grenade->position;
    if (diffX < 16 && diffY < 16 && diffZ < 16 &&
        physics_can_see(server, playerPos.x, playerPos.y, playerPos.z, grenadePos.x, grenadePos.y, grenadePos.z) &&
        grenade->position.z < 62)
    {
        double diff = ((diffX * diffX) + (diffY * diffY) + (diffZ * diffZ));
        if (diff == 0) {
            return 100;
        }
        return (int) fmin((4096.f / diff), 100);
    }
    return 0;
}

void handle_grenade(server_t* server, player_t* player)
{
    grenade_t* grenade;
    grenade_t* tmp;
    if (player->state == STATE_DISCONNECTED) {
        grenade_t* elt;
        uint32_t   counter = 0;
        DL_COUNT(player->grenade, elt, counter);
        if (counter == 0) {
            HASH_DELETE(hh, server->players, player);
            free(player);
            return;
        }
    }
    DL_FOREACH_SAFE(player->grenade, grenade, tmp)
    {
        if (grenade->sent) {
            physics_move_grenade(server, grenade);
            if ((get_nanos() - grenade->time_since_sent) / 1000000000.f >= grenade->fuse) {
                uint8_t allowToDestroy = 0;
                if (grenadeGamemodeCheck(server, grenade->position)) {
                    send_block_action(server,
                                      player,
                                      3,
                                      floor(grenade->position.x),
                                      floor(grenade->position.y),
                                      floor(grenade->position.z));
                    allowToDestroy = 1;
                }
                player_t *connected_player, *tmp_player;
                HASH_ITER(hh, server->players, connected_player, tmp_player)
                {
                    if (connected_player->state == STATE_READY) {
                        uint8_t value = get_grenade_damage(server, connected_player, grenade);
                        if (value > 0) {
                            send_set_hp(server, player, connected_player, value, 1, 3, 5, 1, grenade->position);
                        }
                    }
                }
                float x = grenade->position.x;
                float y = grenade->position.y;
                for (int z = grenade->position.z - 1; z <= grenade->position.z + 1; ++z) {
                    if (z < 62 &&
                        (x >= 0 && x <= server->s_map.map.size_x && x - 1 >= 0 && x - 1 <= server->s_map.map.size_x &&
                         x + 1 >= 0 && x + 1 <= server->s_map.map.size_x) &&
                        (y >= 0 && y <= server->s_map.map.size_y && y - 1 >= 0 && y - 1 <= server->s_map.map.size_y &&
                         y + 1 >= 0 && y + 1 <= server->s_map.map.size_y))
                    {
                        if (allowToDestroy && (z >= 0 && z < server->s_map.map.size_z)) {
                            // This is cause casting float to int produces an edge case where
                            // float < 0 rounds to value closer to 0. Which for -0.(>0) is bad
                            int x_rounded = floorf(x - 1);
                            int y_rounded = floorf(y - 1);
                            for (int X = x_rounded; X < x_rounded + 3; ++X) {
                                for (int Y = y_rounded; Y < y_rounded + 3; ++Y)
                                { // I hate nested loops as any other C dev but here they do not cost that much perf
                                    if (valid_pos_3f(server, X, Y, z))
                                        mapvxl_set_air(&server->s_map.map, X, Y, z);
                                }
                            }
                        }
                        vector3i_t pos;
                        pos.x = grenade->position.x;
                        pos.y = grenade->position.y;
                        pos.z = grenade->position.z;

                        vector3i_t* neigh = getGrenadeNeighbors(pos);

                        for (int index = 0; index < 54; ++index) {
                            if (valid_pos_v3i(server, neigh[index])) {
                                check_node(server, neigh[index]);
                            }
                        }
                    }
                }
                grenade->sent = 0;
                DL_DELETE(player->grenade, grenade);
                free(grenade);
                move_intel_and_tent_down(server);
            }
        }
    }
}
