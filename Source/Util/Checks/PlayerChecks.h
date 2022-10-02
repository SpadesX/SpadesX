#ifndef PLAYERCHECKS_H
#define PLAYERCHECKS_H

#include <Server/Structs/ServerStruct.h>

uint8_t player_to_player_visibile(server_t* server, uint8_t player_id, uint8_t player_id2);
uint8_t is_past_state_data(server_t* server, uint8_t player_id);
uint8_t is_past_join_screen(server_t* server, uint8_t player_id);

#endif
