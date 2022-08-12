// Copyright DarkNeutrino 2021
#include "Protocol.h"
#include "Structs.h"
#include "Util/DataStream.h"
#include "Util/Types.h"
#include "Server.h"

#include <enet/enet.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

void updateMaster(Server* server)
{
    server->protocol.countOfUsers = 0;
    for (int i = 0; i < 32; i++) {
        if (isPastJoinScreen(server, i)) {
            server->protocol.countOfUsers++;
        }
    }
    ENetPacket* packet = enet_packet_create(NULL, 1, ENET_PACKET_FLAG_RELIABLE);
    DataStream  stream = {packet->data, packet->dataLength, 0};
    WriteByte(&stream, server->protocol.countOfUsers);
    enet_peer_send(server->master.peer, 0, packet);
}

int ConnectMaster(Server* server, uint16 port)
{
    server->master.client = enet_host_create(NULL, 1, 1, 0, 0);

    enet_host_compress_with_range_coder(server->master.client);

    if (server->master.client == NULL) {
        fprintf(stderr, "An error occurred while trying to create an ENet client host!\n");
        return EXIT_FAILURE;
    }

    ENetAddress address;

    enet_address_set_host(&address, "67.205.183.163");
    address.port = 32886;

    LOG_STATUS("Connecting to master server");

    server->master.peer = enet_host_connect(server->master.client, &address, 2, 31);
    if (server->master.peer == NULL) {
        fprintf(stderr, "ERROR: failed to create client\n");
        return EXIT_FAILURE;
    }

    ENetEvent event;
    while (enet_host_service(server->master.client, &event, 1000) > 0) {
        LOG_STATUS("Connection success");
        ENetPacket* packet =
        enet_packet_create(NULL,
                           9 + strlen(server->serverName) + strlen(server->gamemodeName) + strlen(server->mapName) + 3,
                           ENET_PACKET_FLAG_RELIABLE);
        DataStream stream = {packet->data, packet->dataLength, 0};
        WriteByte(&stream, 32);
        WriteShort(&stream, port);
        WriteArray(&stream, server->serverName, strlen(server->serverName) + 1);
        WriteArray(&stream, server->gamemodeName, strlen(server->gamemodeName) + 1);
        WriteArray(&stream, server->mapName, strlen(server->mapName) + 1);
        enet_peer_send(server->master.peer, 0, packet);
    }
    return 0;
}

void* keepMasterAlive(void* serverVoid)
{
    Server* server = serverVoid;
    while (server->running) {
        if (server->master.enableMasterConnection == 1) {
            if (time(NULL) - server->master.timeSinceLastSend >= 1) {
                if (enet_host_service(server->master.client, &server->master.event, 0) < 0) {
                    pthread_mutex_lock(&serverLock);
                    LOG_WARNING("Connection to master server lost. Waiting 30 seconds to reconnect...");
                    sleep(30);
                    ConnectMaster(server, server->port);
                    pthread_mutex_unlock(&serverLock);
                }
                server->master.timeSinceLastSend = time(NULL);
            }
        }
        sleep(0);
    }
    enet_host_destroy(server->master.client);
    pthread_exit(0);
}
