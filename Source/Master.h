#ifndef MASTER_H
#define MASTER_H

#include "Structs.h"
#include "Enums.h"
#include "Types.h"

int ConnectMaster(Server* server, uint16 port);
void updateMaster(Server* server);
#endif
