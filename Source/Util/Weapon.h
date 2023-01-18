#ifndef WEAPON_H
#define WEAPON_H

#include <Server/Structs/CommandStruct.h>
#include <stdint.h>

void            set_default_player_ammo(player_t* player);
void            set_default_player_ammo_reserve(player_t* player);
void            set_default_player_pellets(player_t* player);
uint64_t        get_player_weapon_delay_nano(player_t* player);

#endif
