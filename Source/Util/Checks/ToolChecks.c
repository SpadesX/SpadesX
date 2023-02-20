#include <Server/Structs/PlayerStruct.h>
#include <Server/Structs/ServerStruct.h>
#include <Util/Checks/TimeChecks.h>
#include <Util/Checks/ToolChecks.h>
#include <Util/Enums.h>
#include <stdint.h>

uint8_t switch_tool_delay_checks(player_t* player, uint64_t time_now, uint8_t packet_type)
{
    switch (packet_type) {
        case PACKET_TYPE_BLOCK_ACTION:
        case PACKET_TYPE_BLOCK_LINE:
            if (!(((player->item == TOOL_GUN &&
                    diff_is_older_then(time_now, &player->timers.since_last_block_action_weapon, 0)) ||
                   ((player->item == TOOL_SPADE || player->item == TOOL_BLOCK) &&
                    diff_is_older_then(time_now, &player->timers.since_last_block_action, 0))) &&
                  diff_is_older_then_dont_update(
                  time_now, player->timers.since_last_grenade_thrown, NANO_IN_MILLI * 500) &&
                  diff_is_older_then_dont_update(time_now, player->timers.since_last_hit_weapon, NANO_IN_MILLI * 500)))
            {
                return 0;
            }
            break;
        case PACKET_TYPE_GRENADE_PACKET:
            if (!(
                diff_is_older_then_dont_update(time_now, player->timers.since_last_block_action, NANO_IN_MILLI * 500) &&
                diff_is_older_then_dont_update(
                time_now, player->timers.since_last_block_action_weapon, NANO_IN_MILLI * 500) &&
                diff_is_older_then_dont_update(time_now, player->timers.since_last_hit_spade, NANO_IN_MILLI * 500) &&
                diff_is_older_then_dont_update(time_now, player->timers.since_last_hit_weapon, NANO_IN_MILLI * 500)))
            {
                return 0;
            }
            break;
        case PACKET_TYPE_HIT_PACKET:
            if (!(
                ((player->item == TOOL_GUN && diff_is_older_then(time_now, &player->timers.since_last_hit_weapon, 0)) ||
                 (player->item == TOOL_SPADE &&
                  diff_is_older_then(time_now, &player->timers.since_last_hit_spade, 0))) &&
                diff_is_older_then_dont_update(time_now, player->timers.since_last_block_action, NANO_IN_MILLI * 500) &&
                diff_is_older_then_dont_update(
                time_now, player->timers.since_last_grenade_thrown, NANO_IN_MILLI * 500)))
            {
                return 0;
            }
            break;
        case PACKET_TYPE_WEAPON_INPUT:
            if (!(
                diff_is_older_then_dont_update(time_now, player->timers.since_last_block_action, NANO_IN_MILLI * 500) &&
                diff_is_older_then_dont_update(
                time_now, player->timers.since_last_block_action_weapon, NANO_IN_MILLI * 500) &&
                diff_is_older_then_dont_update(
                time_now, player->timers.since_last_grenade_thrown, NANO_IN_MILLI * 500) &&
                diff_is_older_then_dont_update(time_now, player->timers.since_last_hit_spade, NANO_IN_MILLI * 500) &&
                diff_is_older_then_dont_update(time_now, player->timers.since_last_hit_weapon, NANO_IN_MILLI * 500)))
            {
                return 0;
            }
            break;
    }
    return 1;
}
