#ifndef SERVER_H
#define SERVER_H

#include "Enums.h"
#include "Queue.h"
#include "Types.h"
#include "Util/Line.h"
#include "Player.h"
#include "Protocol.h"
//#include "MasterStruct.h"

#include <enet/enet.h>
#include <libvxl/libvxl.h>

#ifndef DEFAULT_SERVER_PORT
    #define DEFAULT_SERVER_PORT 32887
#endif

#define STATUS(msg)  printf("STATUS: " msg "\n")
#define WARNING(msg) printf("WARNING: " msg "\n")
#define ERROR(msg)   fprintf(stderr, "ERROR: " msg "\n");

// Cant have it in Master.h cause functions there require including Server.h which causes errors
typedef struct {
	// Master
    ENetHost* client;
    ENetPeer* peer;
    uint8     enableMasterConnection;
} Master;

typedef struct {
	ENetHost* host;
	Player player[32];
	Protocol protocol;
	Master master;
} Server;

void StartServer(uint16 port, uint32 connections, uint32 channels, uint32 inBandwidth, uint32 outBandwidth, uint8 master);

#endif /* SERVER_H */
