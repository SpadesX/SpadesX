// Copyright DarkNeutrino 2021
#ifndef MASTER_H
#define MASTER_H

#include "Structs.h"
#include "Util/Enums.h"
#include "Util/Types.h"

void* master_keep_alive(void* server);
int   master_connect(server_t* server, uint16_t port);
void  master_update(server_t* server);

#endif
