// Copyright DarkNeutrino 2021
#ifndef MAP_H
#define MAP_H

#include <Server/Structs/ServerStruct.h>
#include <Util/Queue.h>
#include <Util/Types.h>

uint8_t map_load(server_t* server, const char* path, int map_size[3]);

#endif
