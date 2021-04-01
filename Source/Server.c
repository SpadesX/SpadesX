#include "Server.h"

#include <enet/enet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define Z_SOLO
#include <zlib.h>

typedef struct _Node
{
    uint8*        block;
    uint32        size;
    uint32        length;
    struct _Node* next;
} Node;

Node* Queue(Node* node, uint32 size)
{
    Node* next   = (Node*) malloc(sizeof(Node));
    next->next   = NULL;
    next->size   = size;
    next->length = 0;
    next->block  = malloc(size);
    if (node != NULL) {
        node->next = next;
    }
    return next;
}

Node* Pop(Node* node)
{
    if (node != NULL) {
        Node* next = node->next;
        free(node->block);
        free(node);
        return next;
    }
    return NULL;
}

Node* DataCompress(uint8* target, uint32 length)
{
#define CHUNK 8192

    Node* first = NULL;
    Node* node  = NULL;

    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree  = Z_NULL;
    stream.opaque = Z_NULL;
    if (deflateInit(&stream, 5) < 0) {
        printf("error: deflateInit2\n");
        return NULL;
    }

    stream.next_in  = (uint8*) target;
    stream.avail_in = length;

    uint32 offset = 0;
    do {
        if (first == NULL) {
            first = Queue(NULL, CHUNK);
            node  = first;
        } else {
            node = Queue(node, CHUNK);
        }

        // uint32 prev  = 0;
        // uint32 count = 0;
        // while (count < 8192) {
        //     prev    = count;
        //     uint8 N = target[count];
        //     if (N == 0) {
        //         uint8 S = target[count + 1];
        //         uint8 E = target[count + 2];
        //         uint8 L = E - S + 1;
        //         count += 4 * (L + 1);
        //     } else {
        //         count += 4 * N;
        //     }
        // }
        // offset += prev;

        int have;
        // stream.avail_out = prev;
        stream.avail_out = CHUNK;
        stream.next_out  = node->block;

        if (deflate(&stream, Z_FINISH) < 0) {
            printf("error: deflate\n");
            return NULL;
        }

        // have = prev - stream.avail_out;
        have = CHUNK - stream.avail_out;

        node->length = have;
        length -= have;

    } while (stream.avail_out == 0);

    deflateEnd(&stream);

    return first;
}

void ServerStart(Server* server,
                 uint16  port,
                 uint32  connections,
                 uint32  channels,
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

    server->connections    = (Connection*) malloc(sizeof(Connection) * connections);
    server->maxConnections = connections;
    server->event          = (ENetEvent*) malloc(sizeof(ENetEvent));

    enet_host_compress_with_range_coder(server->host);
}

static Node*  GlobalQueue = NULL;
static uint32 FileSize    = 0;

Connection* CurrentConnection = NULL;

void ServerStep(Server* server, int timeout)
{
    ENetPacket* packet = enet_packet_create("ok", 3, ENET_PACKET_FLAG_RELIABLE);

    ENetEvent* event = server->event;
    while (enet_host_service(server->host, event, timeout) > 0) {

        if (event->type == ENET_EVENT_TYPE_DISCONNECT) {
            printf("INFO: connection %X terminated\n", event->peer->address.host);
            Connection* connection = event->peer->data;
            connection->state      = CONNECTION_DISCONNECTED;
            connection->peer       = NULL;
            CurrentConnection      = NULL;
        }

        if (event->type == ENET_EVENT_TYPE_CONNECT) {
            printf("INFO: new connection: %X:%u, channel %u, data %u\n",
                   event->peer->address.host,
                   event->peer->address.port,
                   event->channelID,
                   event->data);

            event->peer->data      = server->connections + 13;
            Connection* connection = event->peer->data;

            connection->playerID = 13;
            connection->state    = CONNECTION_STARTING_MAP;
            connection->peer     = event->peer;

            CurrentConnection = connection;
        }

        if (event->type == ENET_EVENT_TYPE_RECEIVE) {
            printf("PACKET: %X:%u, channel %u, data %u, length %zu\n\tmessage: '%s'\n",
                   event->peer->address.host,
                   event->peer->address.port,
                   event->channelID,
                   event->data,
                   event->packet->dataLength,
                   (const char*) event->packet->data);
        }
    }

    Connection* connection = CurrentConnection;
    if (connection == NULL) {
        return;
    }

    if (connection->state == CONNECTION_STARTING_MAP) {
        printf("STATUS: sending map info\n");

        ENetPacket* packet = enet_packet_create(NULL, 5, ENET_PACKET_FLAG_RELIABLE);
        packet->data[0]    = PACKET_TYPE_MAP_START;
        packet->data[1]    = 0;
        packet->data[2]    = 0;
        packet->data[3]    = 0;
        packet->data[4]    = 0;

        if (GlobalQueue == NULL) {
            printf("STATUS: loading map\n");

            FILE* file = fopen("hallway.vxl", "rb");
            if (!file) {
                perror("file not found");
                exit(EXIT_FAILURE);
            }

            fseek(file, 0, SEEK_END);
            long size = ftell(file);
            fseek(file, 0, SEEK_SET);

            uint8* buffer = (uint8*) malloc(size);
            fread(buffer, size, 1, file);
            fclose(file);

            printf("STATUS: compressing\n");
            GlobalQueue = DataCompress(buffer, size);

            Node* node = GlobalQueue;
            FileSize   = 0;
            while (node) {
                FileSize += GlobalQueue->length;
                node = node->next;
            }

            free(buffer);
        }

        *(uint32*) (packet->data + 1) = FileSize;

        if (enet_peer_send(connection->peer, 0, packet) == 0) {
            connection->state = CONNECTION_LOADING_CHUNKS;
        }
    }

    if (connection->state == CONNECTION_LOADING_CHUNKS) {
        if (GlobalQueue == NULL) {
            connection->state = CONNECTION_LOADING_STATE;
            printf("STATUS: loading chunks done\n");
        } else {
            ENetPacket* packet = enet_packet_create(NULL, GlobalQueue->length + 1, ENET_PACKET_FLAG_RELIABLE);
            packet->data[0]    = PACKET_TYPE_MAP_CHUNK;
            memcpy(packet->data + 1, GlobalQueue->block, GlobalQueue->length);

            printf("sending map data, size: %u\n", GlobalQueue->length);

            if (enet_peer_send(connection->peer, 0, packet) == 0) {
                GlobalQueue = Pop(GlobalQueue);
            }
        }
    }

    if (connection->state == CONNECTION_LOADING_STATE) {
        printf("STATUS: sending state\n");

#define SIZE 53

        ENetPacket* packet = enet_packet_create(NULL, SIZE, ENET_PACKET_FLAG_RELIABLE);
        memset(packet->data, 0, SIZE);

        packet->data[0] = PACKET_TYPE_STATE_DATA;

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
        memcpy(packet->data + 11, "ABCDEFGHIJK", 10);
        // team 2 name
        memcpy(packet->data + 21, "KBCDEFGHIJA", 10);
        // gamemode
        packet->data[31] = 0;
        // CTF STATE:
        //

        if (enet_peer_send(connection->peer, 0, packet) == 0) {
            connection->state = CONNECTION_HOLD;
        }
    }
}

void ServerDestroy(Server* server)
{
    printf("INFO: destroying server\n");

    enet_host_destroy(server->host);
}
