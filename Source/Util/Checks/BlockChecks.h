#ifndef BLOCKCHECKS_H
#define BLOCKCHECKS_H

#include <Server/Structs/CommandStruct.h>
#include <Server/Structs/ServerStruct.h>
#include <stdint.h>

uint8_t block_action_delay_check(server_t* server, player_t* player, uint64_t time_now, uint8_t action_type, uint8_t ignore_weapon);

#endif
