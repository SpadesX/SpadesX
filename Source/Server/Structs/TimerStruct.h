#ifndef TIMERSTRUCT_H
#define TIMERSTRUCT_H

#include <Util/Types.h>

typedef struct timers
{
    uint64_t time_since_last_wu;
    uint64_t since_last_base_enter;
    uint64_t since_last_base_enter_restock;
    uint64_t start_of_respawn_wait;
    uint64_t since_last_shot;
    uint64_t since_previous_shot;
    uint64_t since_last_block_dest_with_gun;
    uint64_t since_last_block_dest;
    uint64_t since_last_block_plac;
    uint64_t since_last_3block_dest;
    uint64_t since_last_grenade_thrown;
    uint64_t since_last_weapon_input;
    uint64_t since_reload_start;
    uint64_t since_last_primary_weapon_input;
    uint64_t since_last_message_from_spam;
    uint64_t since_last_message;
    uint64_t since_possible_spade_nade;
    uint64_t since_periodic_message;
    uint64_t since_last_intel_tent_check;
    uint64_t during_noclip_period;
} timers_t;

typedef struct global_timers
{
    uint64_t update_time;
    uint64_t last_update_time;
    uint64_t time_since_start;
    float    time_since_start_simulated;
} global_timers_t;

#endif
