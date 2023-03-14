#include <libmapvxl/libmapvxl.h>

#include <Server/Staff.h>
#include <Server/Structs/CommandStruct.h>
#include <Server/Structs/PlayerStruct.h>
#include <Util/Checks/BlockChecks.h>
#include <Util/Checks/TimeChecks.h>
#include <Util/Enums.h>
#include <Util/Log.h>
#include <stdint.h>

#define MICRO_ALLOWANCE NANO_IN_MILLI * 5

uint8_t is_block_placable(server_t* server, vector3i_t pos)
{
    static vector3i_t neighbour;

    static const vector3i_t offsets[6] = {{0, 0, -1}, {0, -1, 0}, {0, 1, 0}, {-1, 0, 0}, {1, 0, 0}, {0, 0, 1}};

    for (int i = 0; i < 6; i++) {
        neighbour.x = pos.x + offsets[i].x;
        neighbour.y = pos.y + offsets[i].y;
        neighbour.z = pos.z + offsets[i].z;
        if (mapvxl_is_solid(&server->s_map.map, neighbour.x, neighbour.y, neighbour.z)) {
            return 1;
        }
    }

    return 0;
}

uint8_t block_action_delay_check(server_t* server,
                                 player_t* player,
                                 uint64_t  time_now,
                                 uint8_t   action_type,
                                 uint8_t   ignore_weapon)
{
    // This is bit of a spaghetti. Will be cleaned up later
    if (ignore_weapon == 0 &&
        ((player->weapon == WEAPON_RIFLE &&
          !diff_is_older_then_dont_update(
          time_now, player->timers.since_last_block_dest_with_gun, (RIFLE_DELAY - MICRO_ALLOWANCE))) ||
         (player->weapon == WEAPON_SMG && !diff_is_older_then_dont_update(time_now,
                                                                          player->timers.since_last_block_dest_with_gun,
                                                                          (SMG_DELAY - MICRO_ALLOWANCE))) ||
         (player->weapon == WEAPON_SHOTGUN &&
          !diff_is_older_then_dont_update(
          time_now, player->timers.since_last_block_dest_with_gun, (SHOTGUN_DELAY - MICRO_ALLOWANCE)))))
    {
        send_message_to_staff(server,
                              "Player %s (#%hhu) tried to switch tools and destroy blocks faster then allowed",
                              player->name,
                              player->id);
        return 0;
    }
    switch (action_type) {
        case BLOCKACTION_BUILD:
            if ((diff_is_older_then(time_now, &player->timers.since_last_block_plac, BLOCK_DELAY - MICRO_ALLOWANCE) &&
                 diff_is_older_then_dont_update(
                 time_now, player->timers.since_last_block_dest, BLOCK_DELAY - MICRO_ALLOWANCE) &&
                 diff_is_older_then_dont_update(
                 time_now, player->timers.since_last_3block_dest, BLOCK_DELAY - MICRO_ALLOWANCE)))
            {
                return 1;
            }
            break;
        case BLOCKACTION_DESTROY_ONE:
            if ((diff_is_older_then(time_now, &player->timers.since_last_block_dest, SPADE_DELAY - MICRO_ALLOWANCE) &&
                 diff_is_older_then_dont_update(
                 time_now, player->timers.since_last_3block_dest, SPADE_DELAY - MICRO_ALLOWANCE) &&
                 diff_is_older_then_dont_update(
                 time_now, player->timers.since_last_block_plac, SPADE_DELAY - MICRO_ALLOWANCE)))
            {
                return 1;
            }
            break;
        case BLOCKACTION_DESTROY_THREE:
            if ((diff_is_older_then(
                 time_now, &player->timers.since_last_3block_dest, THREEBLOCK_DELAY - MICRO_ALLOWANCE) &&
                 diff_is_older_then_dont_update(
                 time_now, player->timers.since_last_block_dest, THREEBLOCK_DELAY - MICRO_ALLOWANCE) &&
                 diff_is_older_then_dont_update(
                 time_now, player->timers.since_last_block_plac, THREEBLOCK_DELAY - MICRO_ALLOWANCE)))
            {
                return 1;
            }
            break;
    }
    send_message_to_staff(server, "Player %s (#%hhu) probably has speed destroy hack", player->name, player->id);
    return 0;
}
