#ifndef GAMEMODES_H
#define GAMEMODES_H

#include <Server/Structs/ServerStruct.h>

void    gamemode_init(server_t* server, uint8_t gamemode);
uint8_t gamemode_block_checks(server_t* server, int x, int y, int z);
uint8_t grenadeGamemodeCheck(server_t* server, vector3f_t pos);

#endif
