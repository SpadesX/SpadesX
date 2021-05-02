#ifndef SERVER_H
#define SERVER_H

#include "Enums.h"
#include "Queue.h"
#include "Types.h"
#include "Util/Line.h"
#include "Player.h"
#include "Protocol.h"

#include <enet/enet.h>
#include <libvxl/libvxl.h>

#ifndef DEFAULT_SERVER_PORT
    #define DEFAULT_SERVER_PORT 32887
#endif

#define STATUS(msg)  printf("STATUS: " msg "\n")
#define WARNING(msg) printf("WARNING: " msg "\n")
#define ERROR(msg)   fprintf(stderr, "ERROR: " msg "\n");

typedef struct {
	ENetHost* host;
	Player player[32];
	Protocol protocol;
} Server;

void StartServer(uint16 port, uint32 connections, uint32 channels, uint32 inBandwidth, uint32 outBandwidth);

#endif /* SERVER_H */
