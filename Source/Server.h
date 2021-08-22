//Copyright DarkNeutrino 2021
#ifndef SERVER_H
#define SERVER_H

#include "Enums.h"
#include "Queue.h"
#include "Types.h"
#include "Util/Line.h"
#include "Player.h"
#include "Protocol.h"

#include <enet/enet.h>
#include <libmapvxl/libmapvxl.h>

#ifndef DEFAULT_SERVER_PORT
    #define DEFAULT_SERVER_PORT 32887
#endif

void StartServer(uint16 port, uint32 connections, uint32 channels, uint32 inBandwidth, uint32 outBandwidth, uint8 master, const char* map);
void ServerReset(Server* server, char* map);

#endif /* SERVER_H */
