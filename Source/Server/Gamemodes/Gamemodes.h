#ifndef GAMEMODES_H
#define GAMEMODES_H

#include <Server/Structs/ServerStruct.h>

void    gamemode_init(server_t* server, uint8_t gamemode);
void    gamemode_reset();
// Function called before destroying a block.
uint8_t gamemode_block_deletion_checks(player_t * from, int x, int y, int z);
// Function called before building a block.
uint8_t gamemode_block_creation_checks(player_t * from, int x, int y, int z);

// Function called before destroying blocks with a grenade.
uint8_t grenade_gamemode_check(player_t * from, vector3f_t pos);

#endif
