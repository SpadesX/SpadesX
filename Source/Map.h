#ifndef MAP_H
#define MAP_H

#include "Structs.h"
#include "Queue.h"
#include "Types.h"

void LoadMap(Server* server, const char* path);
void SendMapStart(Server* server, uint8 playerID);
void SendMapChunks(Server* server, uint8 playerID);

#endif
