#include <enet/enet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SERVER_PORT 32887
#define TIMEOUT     10

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

    if (client == NULL) {
        fprintf(stderr, "An error occurred while trying to create an ENet client host!\n");
        return EXIT_FAILURE;
    }

    ENetAddress address;
    ENetPeer*   peer;

    enet_address_set_host(&address, "127.0.0.1");
    address.port = SERVER_PORT;

    printf("INFO: creating client\n");

    peer = enet_host_connect(client, &address, 1, 0);
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
                    printf("PACKET: from %X:%u, channel %u, length %zu:\n%s\n",
                           event.peer->address.host,
                           event.peer->address.port,
                           event.channelID,
                           event.packet->dataLength,
                           event.packet->data);

                    waitAnswer = 0;
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
        if (!waitAnswer) {
            printf("write message:");
            fgets(buffer, MAX_INPUT_LENGTH, stdin);

            int length         = strlen(buffer);
            buffer[length - 1] = '\0'; // replace '\n'

            printf("INFO: sending packet\n");
            // enet_packet_resize(packet, length);
            strcpy((char*) packet->data, buffer);
            enet_peer_send(peer, 0, packet);

            waitAnswer = 1;
        }
    }

    enet_packet_destroy(packet);

    free(buffer);

    printf("INFO: loop end\nINFO: disconnecting/destroying client\n");

    enet_peer_disconnect(peer, 0);
    enet_host_destroy(client);

    return EXIT_SUCCESS;
}
