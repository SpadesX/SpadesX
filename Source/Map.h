// Copyright DarkNeutrino 2021
#ifndef MAP_H
#define MAP_H

#include "Structs.h"

#include <Queue.h>
#include <Types.h>

void LoadMap(Server* server, const char* path);
void writeBlockLine(Server* server, uint8 playerID, vec3i* start, vec3i* end);

#endif
