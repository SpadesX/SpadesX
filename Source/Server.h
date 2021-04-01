#ifndef SERVER_H
#define SERVER_H

#include "Types.h"

#ifndef DEFAULT_SERVER_PORT
    #define DEFAULT_SERVER_PORT 32887
#endif

typedef enum {
    VERSION_0_75 = 3,
    VERSION_0_76 = 4,
} ProtocolVersion;

typedef enum {
    REASON_BANNED                 = 1,
    REASON_IP_LIMIT_EXCEEDED      = 2,
    REASON_WRONG_PROTOCOL_VERSION = 3,
    REASON_SERVER_FULL            = 4,
    REASON_KICKED                 = 10,
} DisconnectedReason;

typedef enum {
    PACKET_TYPE_STATE_DATA = 15,
    PACKET_TYPE_MAP_START  = 18,
    PACKET_TYPE_MAP_CHUNK  = 19,
} PacketID;

typedef enum {
    CONNECTION_STARTING_MAP,
    CONNECTION_LOADING_CHUNKS,
    CONNECTION_LOADING_STATE,
    CONNECTION_HOLD,
    CONNECTION_DISCONNECTED
} ConnectionState;

typedef struct _ENetEvent ENetEvent;
typedef struct _ENetHost  ENetHost;
typedef struct _ENetPeer  ENetPeer;

typedef struct
{
    uint32          playerID;
    ConnectionState state;
    ENetPeer*       peer;
} Connection;

typedef struct
{
    ENetHost*   host;
    ENetEvent*  event;
    Connection* connections;
    uint32      maxConnections;
} Server;

void ServerStart(Server* server,
                 uint16  port,
                 uint32  connections,
                 uint32  channels,
                 uint32  inBandwidth,
                 uint32  outBandwidth);

void ServerStep(Server* server, int timeout);

void ServerDestroy(Server* server);

#endif /* SERVER_H */
