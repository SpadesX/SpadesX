// Copyright DarkNeutrino 2021
#ifndef MASTER_H
#define MASTER_H

#include "Enums.h"
#include "Structs.h"
#include <Types.h>

void* keepMasterAlive(void* server);
int   ConnectMaster(Server* server, uint16 port);
void  updateMaster(Server* server);
#endif
