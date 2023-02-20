#ifndef TOOLCHECKS_H
#define TOOLCHECKS_H

#include <Server/Structs/ServerStruct.h>
#include <stdint.h>

uint8_t switch_tool_delay_checks(player_t* player, uint64_t time_now, uint8_t packet_type);

#endif
