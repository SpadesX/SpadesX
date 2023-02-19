#include <Server/Structs/CommandStruct.h>
#include <Server/Structs/PlayerStruct.h>
#include <Util/Checks/BlockChecks.h>
#include <Util/Checks/TimeChecks.h>
#include <Util/Enums.h>
#include <stdint.h>

uint8_t block_action_delay_check(player_t* player, uint64_t time_now, uint8_t action_type)
{
    switch (action_type) {
        case BLOCKACTION_BUILD:
            if (!(diff_is_older_then(time_now, &player->timers.since_last_block_plac, BLOCK_DELAY) &&
                  diff_is_older_then_dont_update(time_now, player->timers.since_last_block_dest, BLOCK_DELAY) &&
                  diff_is_older_then_dont_update(time_now, player->timers.since_last_3block_dest, BLOCK_DELAY)))
            {
                return 0;
            }
            break;
        case BLOCKACTION_DESTROY_ONE:
            if (!(diff_is_older_then(time_now, &player->timers.since_last_block_dest, SPADE_DELAY) &&
                  diff_is_older_then_dont_update(time_now, player->timers.since_last_3block_dest, SPADE_DELAY) &&
                  diff_is_older_then_dont_update(time_now, player->timers.since_last_block_plac, SPADE_DELAY)))
            {
                return 0;
            }
            break;
        case BLOCKACTION_DESTROY_THREE:
            if (!(diff_is_older_then(time_now, &player->timers.since_last_3block_dest, THREEBLOCK_DELAY) &&
                  diff_is_older_then_dont_update(time_now, player->timers.since_last_block_dest, THREEBLOCK_DELAY) &&
                  diff_is_older_then_dont_update(time_now, player->timers.since_last_block_plac, THREEBLOCK_DELAY)))
            {
                return 0;
            }
            break;
    }
    return 1;
}
