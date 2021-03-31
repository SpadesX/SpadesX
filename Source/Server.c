#include "enet/types.h"

#include <enet/enet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SERVER_PORT             32887
#define MAX_CONNECTIONS         32
#define MAX_CHANNELS            1
#define MAX_INCOMING_BANDWIDTH  0
#define MAX_OUTCOMING_BANDWIDTH 0
#define TIMEOUT                 10

#define VERSION_0_75 3
#define VERSION_0_76 4

#define REASON_BANNED                 1
#define REASON_IP_LIMIT_EXCEEDED      2
#define REASON_WRONG_PROTOCOL_VERSION 3
#define REASON_SERVER_FULL            4
#define REASON_KICKED                 10

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;

typedef enum {
    PACKET_TYPE_STATE_DATA = 15,
    PACKET_TYPE_MAP_START  = 18,
    PACKET_TYPE_MAP_CHUNK  = 19,
} PacketID;

typedef enum {
    CONNECTION_STARTING_MAP,
    CONNECTION_SENDING_CHUNKS,
    CONNECTION_SENDING_STATE,
    CONNECTION_HOLD,
} ConnectionState;

typedef struct
{
    uint32          playerID;
    ConnectionState state;
} Connection;

typedef struct
{
    ENetHost* host;
} Server;

void ServerStart(Server* server,
                 uint16  port,
                 size_t  connections,
                 size_t  channels,
                 uint32  inBandwidth,
                 uint32  outBandwidth)
{
    printf("INFO: initializing enet\n");

    if (enet_initialize() != 0) {
        fprintf(stderr, "ERROR: failed to initialize enet\n");
        exit(EXIT_FAILURE);
    }
    atexit(enet_deinitialize);

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;

    printf("INFO: creating server\n");

    server->host = enet_host_create(&address, connections, channels, inBandwidth, outBandwidth);
    if (server == NULL) {
        fprintf(stderr, "ERROR: failed to create server\n");
        exit(EXIT_FAILURE);
    }

    enet_host_compress_with_range_coder(server->host);
}

void ServerStep(Server* server, int timeout)
{
    ENetEvent   event;
    ENetPacket* packet = enet_packet_create("ok", 3, ENET_PACKET_FLAG_RELIABLE);

    while (enet_host_service(server->host, &event, timeout) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                printf("INFO: new connection: %X:%u, channel %u, data %u\n",
                       event.peer->address.host,
                       event.peer->address.port,
                       event.channelID,
                       event.data);

                if (event.data != VERSION_0_75) {
                    printf("client invalid version, disconnecting\n");
                    enet_peer_disconnect(event.peer, REASON_WRONG_PROTOCOL_VERSION);
                }

                Connection* connection = (Connection*) malloc(sizeof(Connection));
                connection->state      = CONNECTION_STARTING_MAP;
                connection->playerID   = 13;
                event.peer->data       = connection;
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                printf("PACKET: %X:%u, channel %u, data %u, length %zu\n\tmessage: '%s'\n",
                       event.peer->address.host,
                       event.peer->address.port,
                       event.channelID,
                       event.data,
                       event.packet->dataLength,
                       (const char*) event.packet->data);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                printf("INFO: connection %X terminated\n", event.peer->address.host);
                free(event.peer->data);
                break;
            case ENET_EVENT_TYPE_NONE:
                printf("WARNING: empty event\n");
                break;
        }

        Connection* connection = (Connection*) event.peer->data;

        if (connection->state == CONNECTION_STARTING_MAP) {
            printf("STATUS: sending map info\n");

            ENetPacket* packet = enet_packet_create(NULL, 5, ENET_PACKET_FLAG_RELIABLE);
            packet->data[0]    = PACKET_TYPE_MAP_START;
            packet->data[1]    = 0;
            packet->data[2]    = 0;
            packet->data[3]    = 0;
            packet->data[4]    = 0;

            if (enet_peer_send(event.peer, 0, packet) == 0) {
                connection->state = CONNECTION_SENDING_CHUNKS;
            }
        }

        if (connection->state == CONNECTION_SENDING_CHUNKS) {
            printf("STATUS: sending map data\n");

            uint32 packetSize = 2;

            ENetPacket* packet = enet_packet_create(NULL, packetSize, ENET_PACKET_FLAG_RELIABLE);
            packet->data[0]    = PACKET_TYPE_MAP_CHUNK;
            packet->data[1]    = 0;

            if (enet_peer_send(event.peer, 0, packet) == 0) {
                connection->state = CONNECTION_SENDING_STATE;
            }
        }

        if (connection->state == CONNECTION_SENDING_STATE) {
            printf("STATUS: sending state\n");

            ENetPacket* packet = enet_packet_create(NULL, 52, ENET_PACKET_FLAG_RELIABLE);
            memset(packet->data, 0, 52);

            packet->data[0] = PACKET_TYPE_MAP_CHUNK;

            // player id
            packet->data[1] = connection->playerID;

            // fog:
            packet->data[2] = 0xff;
            packet->data[3] = 0xff;
            packet->data[4] = 0xff;

            // team 1 colour
            packet->data[5] = 0xff;
            packet->data[6] = 0x00;
            packet->data[7] = 0x00;

            // team 2 colour
            packet->data[8]  = 0x00;
            packet->data[9]  = 0xff;
            packet->data[10] = 0x00;

            // team 1 name
            packet->data[11] = 'A';
            packet->data[12] = '\0';
            // + 8

            // team 2 name
            packet->data[20] = 'B';
            packet->data[21] = '\0';

            // gamemode
            packet->data[29] = 0;

            if (enet_peer_send(event.peer, 0, packet) == 0) {
                connection->state = CONNECTION_HOLD;
            }
        }

        printf("sending state\n");
        enet_packet_resize(packet, 52);
        packet->data[0] = 15;

        enet_peer_send(event.peer, 0, packet);
    }
}

void ServerDestroy(Server* server)
{
    printf("INFO: destroying server\n");

    enet_host_destroy(server->host);
}

int main()
{
    Server server;
    ServerStart(&server, SERVER_PORT, 32, 12, 0, 0);

    printf("INFO: starting loop\n");

    int stopRunning = 0;
    while (!stopRunning) {
        ServerStep(&server, 1);
    }

    printf("INFO: loop end\n");

    ServerDestroy(&server);
    return EXIT_SUCCESS;
}
