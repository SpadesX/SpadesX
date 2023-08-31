#include "Util/Log.h"

#include <Server/Block.h>
#include <Server/Gamemodes/Gamemodes.h>
#include <Server/IntelTent.h>
#include <Server/Nodes.h>
#include <Server/Packets/Packets.h>
#include <Server/Staff.h>
#include <Util/Checks/BlockChecks.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/Checks/PositionChecks.h>
#include <Util/Checks/TimeChecks.h>
#include <Util/Checks/WeaponChecks.h>
#include <Util/Enums.h>
#include <Util/Nanos.h>

static void
block_action_build(server_t* server, player_t* player, uint8_t action_type, uint32_t X, uint32_t Y, uint32_t Z)
{
    uint64_t time_now = get_nanos();
    vector3i_t position = {X, Y, Z};
    if (!(gamemode_block_checks(server,player, X, Y, Z) && player->blocks > 0 &&
        block_action_delay_check(server, player, time_now, action_type, 1) && is_block_placable(server, position)))
    {
        return;
    }
    mapvxl_set_color(&server->s_map.map, X, Y, Z, player->tool_color.raw);
    player->blocks--;
    moveIntelAndTentUp(server);
    send_block_action(server, player, action_type, X, Y, Z);
}

static void
block_action_destroy_one(server_t* server, player_t* player, uint8_t action_type, uint32_t X, uint32_t Y, uint32_t Z)
{
    uint64_t time_now = get_nanos();
    if (!((Z < 62 && gamemode_block_checks(server,player, X, Y, Z)) &&
          ((player->item == TOOL_SPADE && block_action_delay_check(server, player, time_now, action_type, 0)) ||
          (player->item == TOOL_GUN && block_action_weapon_checks(server, player, time_now)))))
    {
        return;
    }

    vector3i_t  position = {X, Y, Z};
    vector3i_t* neigh    = get_neighbours(position);
    mapvxl_set_air(&server->s_map.map, position.x, position.y, position.z);
    for (int i = 0; i < 6; ++i) {
        if (neigh[i].z < 62) {
            check_node(server, neigh[i]);
        }
    }
    if (player->item != TOOL_GUN) {
        if (player->blocks < 50) {
            player->blocks++;
        }
    }
    send_block_action(server, player, action_type, X, Y, Z);
}

static void
block_action_destroy_three(server_t* server, player_t* player, uint8_t action_type, uint32_t X, uint32_t Y, uint32_t Z)
{
    uint64_t time_now = get_nanos();
    if (!(gamemode_block_checks(server, player, X, Y, Z) && gamemode_block_checks(server,  player, X, Y, Z + 1) &&
        gamemode_block_checks(server,  player, X, Y, Z - 1) && block_action_delay_check(server, player, time_now, action_type, 1)) || player->item == TOOL_GUN)
    {
        return;
    }
    for (uint32_t z = Z - 1; z <= Z + 1; z++) {
        if (z >= 62) {
            continue;
        }
        mapvxl_set_air(&server->s_map.map, X, Y, z);
        vector3i_t  position = {X, Y, z};
        vector3i_t* neigh    = get_neighbours(position);
        mapvxl_set_air(&server->s_map.map, position.x, position.y, position.z);
        for (int i = 0; i < 6; ++i) {
            if (neigh[i].z < 62) {
                check_node(server, neigh[i]);
            }
        }
    }
    send_block_action(server, player, action_type, X, Y, Z);
}

void handle_block_action(server_t*  server,
                         player_t*  player,
                         uint8_t    action_type,
                         vector3i_t vector_block,
                         vector3f_t vectorf_block,
                         vector3f_t player_vector,
                         uint32_t   X,
                         uint32_t   Y,
                         uint32_t   Z)
{
    if ((distance_in_3d(vectorf_block, player_vector) > 4 && player->item != TOOL_GUN) ||
        !valid_pos_v3i(server, vector_block))
    {
        return;
    }
    switch (action_type) {
        case BLOCKACTION_BUILD:
        {
            block_action_build(server, player, action_type, X, Y, Z);
        } break;

        case BLOCKACTION_DESTROY_ONE:
        {
            block_action_destroy_one(server, player, action_type, X, Y, Z);
        } break;

        case BLOCKACTION_DESTROY_THREE:
        {
            block_action_destroy_three(server, player, action_type, X, Y, Z);
        } break;
    }
}
