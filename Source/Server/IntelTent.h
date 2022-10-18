#ifndef INTELTENT_H
#define INTELTENT_H

#include <Server/Structs/ServerStruct.h>

uint8_t    check_under_tent(server_t* server, uint8_t team);
uint8_t    check_under_intel(server_t* server, uint8_t team);
uint8_t    check_in_tent(server_t* server, uint8_t team);
uint8_t    check_in_intel(server_t* server, uint8_t team);
uint8_t    check_player_in_tent(server_t* server, player_t* player);
uint8_t    check_item_on_intel(server_t* server, uint8_t team, vector3f_t itemPos);
void       move_intel_and_tent_down(server_t* server);
void       moveIntelAndTentUp(server_t* server);
uint8_t    check_player_on_intel(server_t* server, player_t* player, uint8_t team);
uint8_t    check_item_in_tent(server_t* server, uint8_t team, vector3f_t itemPos);
vector3f_t set_intel_tent_spawn_point(server_t* server, uint8_t team);
void       handleTentAndIntel(server_t* server, player_t* player);

#endif
