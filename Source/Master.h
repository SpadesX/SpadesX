// Copyright DarkNeutrino 2021
#ifndef MASTER_H
#define MASTER_H

#include "Structs.h"
#include "Util/Enums.h"
#include "Util/Types.h"

void keepMasterAlive(void* server);
int   ConnectMaster(Server* server, uint16 port);
void  updateMaster(Server* server);
#endif
