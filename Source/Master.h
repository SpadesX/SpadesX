//Copyright DarkNeutrino 2021
#ifndef MASTER_H
#define MASTER_H

#include "Structs.h"
#include "Enums.h"
#include "Types.h"

void keepMasterAlive(Server* server);
int ConnectMaster(Server* server, uint16 port);
void updateMaster(Server* server);
#endif
