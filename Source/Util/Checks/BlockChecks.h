#ifndef BLOCKCHECKS_H
#define BLOCKCHECKS_H

#include <Server/Structs/CommandStruct.h>
#include <stdint.h>

uint8_t block_action_delay_check(player_t* player, uint64_t time_now, uint8_t action_type);

#endif
