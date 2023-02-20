#ifndef WEAPONCHECKS_H
#define WEAPONCHECKS_H

#include <Server/Structs/ServerStruct.h>
#include <stdint.h>

uint8_t block_action_weapon_checks(server_t* server, player_t* player, uint64_t time_now);

#endif
