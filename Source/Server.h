#ifndef SERVER_H
#define SERVER_H

#include <Types.h>

#ifndef DEFAULT_SERVER_PORT
    #define DEFAULT_SERVER_PORT 32887
#endif
typedef struct GameServer;
void ServerRun(uint16 port, uint32 connections, uint32 channels, uint32 inBandwidth, uint32 outBandwidth);

#endif /* SERVER_H */
