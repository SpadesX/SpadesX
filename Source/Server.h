// Copyright DarkNeutrino 2021
#ifndef SERVER_H
#define SERVER_H

#include "Protocol.h"
#include "Util/Enums.h"
#include "Util/Line.h"
#include "Util/Queue.h"
#include "Util/Types.h"

#include <enet/enet.h>
#include <libmapvxl/libmapvxl.h>

#ifndef DEFAULT_SERVER_PORT
    #define DEFAULT_SERVER_PORT 32887
#endif

Server* getServer();

void StartServer(uint16      port,
                 uint32      connections,
                 uint32      channels,
                 uint32      inBandwidth,
                 uint32      outBandwidth,
                 uint8       master,
                 stringNode* mapList,
                 uint8       mapCount,
                 stringNode* welcomeMessageList,
                 uint8       welcomeMessageListLen,
                 stringNode* periodicMessageList,
                 uint8       periodicMessageListLen,
                 uint8*      periodicDelays,
                 const char* managerPasswd,
                 const char* adminPasswd,
                 const char* modPasswd,
                 const char* guardPasswd,
                 const char* trustedPasswd,
                 const char* serverName,
                 const char* team1Name,
                 const char* team2Name,
                 uint8*      team1Color,
                 uint8*      team2Color,
                 uint8       gamemode);
void ServerReset(Server* server);

#endif /* SERVER_H */
