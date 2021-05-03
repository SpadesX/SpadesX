#ifndef PLAYER_H
#define PLAYER_H

#include "Structs.h"
#include "Types.h"

void SendPlayerState(Server* server, uint8 playerID, uint8 otherID);

#endif
