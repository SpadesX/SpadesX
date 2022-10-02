#include <Server/Gamemodes/Gamemodes.h>
#include <Server/IntelTent.h>
#include <Server/Nodes.h>
#include <Server/Packets/Packets.h>
#include <Server/Structs/ServerStruct.h>
#include <Util/Checks/PositionChecks.h>
#include <Util/Nanos.h>
#include <Util/Physics.h>
#include <Util/Utlist.h>
#include <math.h>

vector3i_t* getGrenadeNeighbors(vector3i_t pos)
{
    static vector3i_t neighArray[54];
    // This is nasty but fast.
    uint8_t index = 0;
    //-z size
    neighArray[index].x = pos.x;
    neighArray[index].y = pos.y;
    neighArray[index].z = pos.z - 2;
    index++;
    neighArray[index].x = pos.x + 1;
    neighArray[index].y = pos.y;
    neighArray[index].z = pos.z - 2;
    index++;
    neighArray[index].x = pos.x - 1;
    neighArray[index].y = pos.y;
    neighArray[index].z = pos.z - 2;
    index++;
    neighArray[index].x = pos.x;
    neighArray[index].y = pos.y + 1;
    neighArray[index].z = pos.z - 2;
    index++;
    neighArray[index].x = pos.x;
    neighArray[index].y = pos.y - 1;
    neighArray[index].z = pos.z - 2;
    index++;
    neighArray[index].x = pos.x + 1;
    neighArray[index].y = pos.y + 1;
    neighArray[index].z = pos.z - 2;
    index++;
    neighArray[index].x = pos.x + 1;
    neighArray[index].y = pos.y - 1;
    neighArray[index].z = pos.z - 2;
    index++;
    neighArray[index].x = pos.x - 1;
    neighArray[index].y = pos.y + 1;
    neighArray[index].z = pos.z - 2;
    index++;
    neighArray[index].x = pos.x - 1;
    neighArray[index].y = pos.y - 1;
    neighArray[index].z = pos.z - 2;
    index++;
    //-y side
    neighArray[index].x = pos.x;
    neighArray[index].y = pos.y - 2;
    neighArray[index].z = pos.z;
    index++;
    neighArray[index].x = pos.x + 1;
    neighArray[index].y = pos.y - 2;
    neighArray[index].z = pos.z;
    index++;
    neighArray[index].x = pos.x - 1;
    neighArray[index].y = pos.y - 2;
    neighArray[index].z = pos.z;
    index++;
    neighArray[index].x = pos.x;
    neighArray[index].y = pos.y - 2;
    neighArray[index].z = pos.z + 1;
    index++;
    neighArray[index].x = pos.x;
    neighArray[index].y = pos.y - 2;
    neighArray[index].z = pos.z - 1;
    index++;
    neighArray[index].x = pos.x + 1;
    neighArray[index].y = pos.y - 2;
    neighArray[index].z = pos.z + 1;
    index++;
    neighArray[index].x = pos.x + 1;
    neighArray[index].y = pos.y - 2;
    neighArray[index].z = pos.z - 1;
    index++;
    neighArray[index].x = pos.x - 1;
    neighArray[index].y = pos.y - 2;
    neighArray[index].z = pos.z + 1;
    index++;
    neighArray[index].x = pos.x - 1;
    neighArray[index].y = pos.y - 2;
    neighArray[index].z = pos.z - 1;
    index++;
    //+y side
    neighArray[index].x = pos.x;
    neighArray[index].y = pos.y + 2;
    neighArray[index].z = pos.z;
    index++;
    neighArray[index].x = pos.x + 1;
    neighArray[index].y = pos.y + 2;
    neighArray[index].z = pos.z;
    index++;
    neighArray[index].x = pos.x - 1;
    neighArray[index].y = pos.y + 2;
    neighArray[index].z = pos.z;
    index++;
    neighArray[index].x = pos.x;
    neighArray[index].y = pos.y + 2;
    neighArray[index].z = pos.z + 1;
    index++;
    neighArray[index].x = pos.x;
    neighArray[index].y = pos.y + 2;
    neighArray[index].z = pos.z - 1;
    index++;
    neighArray[index].x = pos.x + 1;
    neighArray[index].y = pos.y + 2;
    neighArray[index].z = pos.z + 1;
    index++;
    neighArray[index].x = pos.x + 1;
    neighArray[index].y = pos.y + 2;
    neighArray[index].z = pos.z - 1;
    index++;
    neighArray[index].x = pos.x - 1;
    neighArray[index].y = pos.y + 2;
    neighArray[index].z = pos.z + 1;
    index++;
    neighArray[index].x = pos.x - 1;
    neighArray[index].y = pos.y + 2;
    neighArray[index].z = pos.z - 1;
    index++;
    //-x side
    neighArray[index].x = pos.x - 2;
    neighArray[index].y = pos.y;
    neighArray[index].z = pos.z;
    index++;
    neighArray[index].x = pos.x - 2;
    neighArray[index].y = pos.y + 1;
    neighArray[index].z = pos.z;
    index++;
    neighArray[index].x = pos.x - 2;
    neighArray[index].y = pos.y - 1;
    neighArray[index].z = pos.z;
    index++;
    neighArray[index].x = pos.x - 2;
    neighArray[index].y = pos.y;
    neighArray[index].z = pos.z + 1;
    index++;
    neighArray[index].x = pos.x - 2;
    neighArray[index].y = pos.y;
    neighArray[index].z = pos.z - 1;
    index++;
    neighArray[index].x = pos.x - 2;
    neighArray[index].y = pos.y + 1;
    neighArray[index].z = pos.z + 1;
    index++;
    neighArray[index].x = pos.x - 2;
    neighArray[index].y = pos.y + 1;
    neighArray[index].z = pos.z - 1;
    index++;
    neighArray[index].x = pos.x - 2;
    neighArray[index].y = pos.y - 1;
    neighArray[index].z = pos.z - 1;
    index++;
    neighArray[index].x = pos.x - 2;
    neighArray[index].y = pos.y - 1;
    neighArray[index].z = pos.z + 1;
    index++;
    //+x side
    neighArray[index].x = pos.x + 2;
    neighArray[index].y = pos.y;
    neighArray[index].z = pos.z;
    index++;
    neighArray[index].x = pos.x + 2;
    neighArray[index].y = pos.y + 1;
    neighArray[index].z = pos.z;
    index++;
    neighArray[index].x = pos.x + 2;
    neighArray[index].y = pos.y - 1;
    neighArray[index].z = pos.z;
    index++;
    neighArray[index].x = pos.x + 2;
    neighArray[index].y = pos.y;
    neighArray[index].z = pos.z + 1;
    index++;
    neighArray[index].x = pos.x + 2;
    neighArray[index].y = pos.y;
    neighArray[index].z = pos.z - 1;
    index++;
    neighArray[index].x = pos.x + 2;
    neighArray[index].y = pos.y + 1;
    neighArray[index].z = pos.z + 1;
    index++;
    neighArray[index].x = pos.x + 2;
    neighArray[index].y = pos.y + 1;
    neighArray[index].z = pos.z - 1;
    index++;
    neighArray[index].x = pos.x + 2;
    neighArray[index].y = pos.y - 1;
    neighArray[index].z = pos.z - 1;
    index++;
    neighArray[index].x = pos.x + 2;
    neighArray[index].y = pos.y - 1;
    neighArray[index].z = pos.z + 1;
    index++;
    //+z side
    neighArray[index].x = pos.x;
    neighArray[index].y = pos.y;
    neighArray[index].z = pos.z + 2;
    index++;
    neighArray[index].x = pos.x + 1;
    neighArray[index].y = pos.y;
    neighArray[index].z = pos.z + 2;
    index++;
    neighArray[index].x = pos.x - 1;
    neighArray[index].y = pos.y;
    neighArray[index].z = pos.z + 2;
    index++;
    neighArray[index].x = pos.x;
    neighArray[index].y = pos.y + 1;
    neighArray[index].z = pos.z + 2;
    index++;
    neighArray[index].x = pos.x;
    neighArray[index].y = pos.y - 1;
    neighArray[index].z = pos.z + 2;
    index++;
    neighArray[index].x = pos.x + 1;
    neighArray[index].y = pos.y + 1;
    neighArray[index].z = pos.z + 2;
    index++;
    neighArray[index].x = pos.x + 1;
    neighArray[index].y = pos.y - 1;
    neighArray[index].z = pos.z + 2;
    index++;
    neighArray[index].x = pos.x - 1;
    neighArray[index].y = pos.y + 1;
    neighArray[index].z = pos.z + 2;
    index++;
    neighArray[index].x = pos.x - 1;
    neighArray[index].y = pos.y - 1;
    neighArray[index].z = pos.z + 2;
    index++;

    return neighArray;
}

uint8_t get_grenade_damage(server_t* server, uint8_t damageID, grenade_t* grenade)
{
    double     diffX      = fabs(server->player[damageID].movement.position.x - grenade->position.x);
    double     diffY      = fabs(server->player[damageID].movement.position.y - grenade->position.y);
    double     diffZ      = fabs(server->player[damageID].movement.position.z - grenade->position.z);
    vector3f_t playerPos  = server->player[damageID].movement.position;
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

void handle_grenade(server_t* server, uint8_t player_id)
{
    grenade_t* grenade;
    grenade_t* tmp;
    DL_FOREACH_SAFE(server->player[player_id].grenade, grenade, tmp)
    {
        if (grenade->sent) {
            physics_move_grenade(server, grenade);
            if ((get_nanos() - grenade->time_since_sent) / 1000000000.f >= grenade->fuse) {
                uint8_t allowToDestroy = 0;
                if (grenadeGamemodeCheck(server, grenade->position)) {
                    send_block_action(server,
                                      player_id,
                                      3,
                                      floor(grenade->position.x),
                                      floor(grenade->position.y),
                                      floor(grenade->position.z));
                    allowToDestroy = 1;
                }
                for (int y = 0; y < server->protocol.max_players; ++y) {
                    if (server->player[y].state == STATE_READY) {
                        uint8_t value = get_grenade_damage(server, y, grenade);
                        if (value > 0) {
                            send_set_hp(server, player_id, y, value, 1, 3, 5, 1, grenade->position);
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
                        if (allowToDestroy) {
                            mapvxl_set_air(&server->s_map.map, x - 1, y - 1, z);
                            mapvxl_set_air(&server->s_map.map, x, y - 1, z);
                            mapvxl_set_air(&server->s_map.map, x + 1, y - 1, z);
                            mapvxl_set_air(&server->s_map.map, x - 1, y, z);
                            mapvxl_set_air(&server->s_map.map, x, y, z);
                            mapvxl_set_air(&server->s_map.map, x + 1, y, z);
                            mapvxl_set_air(&server->s_map.map, x - 1, y + 1, z);
                            mapvxl_set_air(&server->s_map.map, x, y + 1, z);
                            mapvxl_set_air(&server->s_map.map, x + 1, y + 1, z);
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
                DL_DELETE(server->player[player_id].grenade, grenade);
                free(grenade);
                move_intel_and_tent_down(server);
            }
        }
    }
}
