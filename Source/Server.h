// Copyright DarkNeutrino 2021
#ifndef SERVER_H
#define SERVER_H

#include "Enums.h"
#include "Protocol.h"
#include "Util/Line.h"

#include <Queue.h>
#include <Types.h>
#include <enet/enet.h>
#include <libmapvxl/libmapvxl.h>

#ifndef DEFAULT_SERVER_PORT
    #define DEFAULT_SERVER_PORT 32887
#endif

void StartServer(uint16      port,
                 uint32      connections,
                 uint32      channels,
                 uint32      inBandwidth,
                 uint32      outBandwidth,
                 uint8       master,
                 char        mapArray[][64],
                 uint8       mapCount,
                 const char* managerPasswd,
                 const char* adminPasswd,
                 const char* modPasswd,
                 const char* guardPasswd,
                 const char* trustedPasswd,
                 const char*       serverName,
                 const char*       team1Name,
                 const char*       team2Name,
                 uint8*      team1Color,
                 uint8*      team2Color,
                 uint8       gamemode);
void ServerReset(Server* server);

#endif /* SERVER_H */
