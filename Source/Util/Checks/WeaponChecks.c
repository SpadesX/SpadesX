#include <Server/Staff.h>
#include <Server/Structs/ServerStruct.h>
#include <Server/Structs/PlayerStruct.h>
#include <Util/Checks/TimeChecks.h>
#include <Util/Checks/WeaponChecks.h>
#include <Util/Enums.h>
#include <stdint.h>

uint8_t block_action_weapon_checks(server_t* server, player_t* player, uint64_t time_now)
{
    if (player->weapon_clip == 0 &&
        !diff_is_older_then_dont_update(time_now, player->timers.since_last_primary_weapon_input, 10 * NANO_IN_MILLI))
    {
        send_message_to_staff(
        server, "Player %s (#%hhu) probably has hack to have more ammo", player->name, player->id);
        return 0;
    } else if (player->weapon == WEAPON_RIFLE && diff_is_older_then(time_now,
                                                                    &player->timers.since_last_block_dest_with_gun,
                                                                    (RIFLE_DELAY - (NANO_IN_MILLI * 10))) == 0)
    {
        send_message_to_staff(server, "Player %s (#%hhu) probably has rapid shooting hack", player->name, player->id);
        return 0;
    } else if (player->weapon == WEAPON_SMG && diff_is_older_then(time_now,
                                                                  &player->timers.since_last_block_dest_with_gun,
                                                                  (SMG_DELAY - (NANO_IN_MILLI * 10))) == 0)
    {
        send_message_to_staff(server, "Player %s (#%hhu) probably has rapid shooting hack", player->name, player->id);
        return 0;
    } else if (player->weapon == WEAPON_SHOTGUN && diff_is_older_then(time_now,
                                                                      &player->timers.since_last_block_dest_with_gun,
                                                                      (SHOTGUN_DELAY - (NANO_IN_MILLI * 10))) == 0)
    {
        send_message_to_staff(server, "Player %s (#%hhu) probably has rapid shooting hack", player->name, player->id);
        return 0;
    }
    return 1;
}
