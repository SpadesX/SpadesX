#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "Enums.h"
#include "Queue.h"
#include "Types.h"
#include "Map.h"

#include <enet/enet.h>

void SendStateData(Server* server, uint8 playerID);

#endif
