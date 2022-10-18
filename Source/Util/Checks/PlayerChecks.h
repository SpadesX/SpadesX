#ifndef PLAYERCHECKS_H
#define PLAYERCHECKS_H

#include <Server/Structs/PlayerStruct.h>

uint8_t player_to_player_visibile(player_t* player, player_t* player2);
uint8_t is_past_state_data(player_t* player);
uint8_t is_past_join_screen(player_t* player);

#endif
