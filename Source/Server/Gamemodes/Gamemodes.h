#ifndef GAMEMODES_H
#define GAMEMODES_H

#include <Server/Structs/ServerStruct.h>

void gamemode_init(server_t* server, uint8_t gamemode);
void gamemode_reset();
void gamemode_on_player_join(player_t* player);
void gamemode_on_player_left(player_t* player);
// Function called before destroying a block.
uint8_t gamemode_hand_destruction_checks(player_t* from, uint16_t x, uint16_t y, uint16_t z);
// Function called before destroying a block.
uint8_t gamemode_gun_destruction_checks(player_t* from, uint16_t x, uint16_t y, uint16_t z);
// Function called before building a block.
uint8_t gamemode_block_creation_checks(player_t* from, uint16_t x, uint16_t y, uint16_t z);
// Function called before destroying blocks with a grenade.
uint8_t gamemode_grenade_destruction_checks(player_t* from, vector3f_t pos);

#endif
