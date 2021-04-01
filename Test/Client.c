#include <enet/enet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SERVER_PORT 32887
#define TIMEOUT     1

#define MAX_INPUT_LENGTH 100

int main()
{
    printf("INFO: initializing enet\n");

    if (enet_initialize() != 0) {
        fprintf(stderr, "ERROR: failed to initialize enet\n");
        return EXIT_FAILURE;
    }
    atexit(enet_deinitialize);

    ENetHost* client;
    client = enet_host_create(NULL, 1, 1, 0, 0);

    enet_host_compress_with_range_coder(client);

    if (client == NULL) {
        fprintf(stderr, "An error occurred while trying to create an ENet client host!\n");
        return EXIT_FAILURE;
    }

    ENetAddress address;
    ENetPeer*   peer;

    enet_address_set_host(&address, "127.0.0.1");
    address.port = SERVER_PORT;

    printf("INFO: creating client\n");

    peer = enet_host_connect(client, &address, 1, 3);
    if (peer == NULL) {
        fprintf(stderr, "ERROR: failed to create client\n");
        return EXIT_FAILURE;
    }

    printf("INFO: trying to connect\n");

    ENetEvent event;
    if (enet_host_service(client, &event, 2000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
        printf("INFO: connection established\n");
    } else {
        enet_peer_reset(peer);
        printf("INFO: failed to establish connection\n");
        return EXIT_SUCCESS;
    }

    printf("INFO: starting loop\n");

    ENetPacket* packet = enet_packet_create(NULL, MAX_INPUT_LENGTH, ENET_PACKET_FLAG_RELIABLE);

    char* buffer = (char*) malloc(MAX_INPUT_LENGTH);

    int stopRunning = 0;
    int waitAnswer  = 0;
    while (!stopRunning) {
        while (enet_host_service(client, &event, TIMEOUT) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_RECEIVE:
                    printf("PACKET: from %X:%u, channel %u, length %zu, code: %u\n",
                           event.peer->address.host,
                           event.peer->address.port,
                           event.channelID,
                           event.packet->dataLength,
                           event.packet->data[0]);
                    if (event.packet->data[0] == 19) {
                        break;
                    }
                    unsigned  length = event.packet->dataLength;
                    unsigned  words  = length / 4;
                    unsigned  bytes  = length % 4;
                    unsigned* data   = (unsigned*) event.packet->data;
                    for (unsigned i = 0; i < words; ++i) {
                        printf("%X", data[i]);
                    }
                    char* ptr = (char*) (data + words);
                    for (unsigned i = 0; i < bytes; ++i) {
                        printf("%X", ptr[i]);
                    }
                    putchar('\n');
                    // enet_packet_destroy(event.packet);
                    break;
                case ENET_EVENT_TYPE_DISCONNECT:
                    printf("INFO: disconnected\n");
                    stopRunning = 1;
                    break;
                default:
                    printf("WARNING: invalid event\n");
                    break;
            }
        }
    }

    enet_packet_destroy(packet);

    free(buffer);

    printf("INFO: loop end\nINFO: disconnecting/destroying client\n");

    enet_peer_disconnect(peer, 0);
    enet_host_destroy(client);

    return EXIT_SUCCESS;
}
