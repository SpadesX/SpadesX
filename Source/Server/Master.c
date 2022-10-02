// Copyright DarkNeutrino 2021
#include <Server/Server.h>
#include <Server/Structs/ServerStruct.h>
#include <Util/Checks/PlayerChecks.h>
#include <Util/DataStream.h>
#include <Util/Log.h>
#include <Util/Types.h>
#include <enet/enet.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

void master_update(server_t* server)
{
    server->protocol.num_users = 0;
    for (int i = 0; i < 32; i++) {
        if (is_past_join_screen(server, i)) {
            server->protocol.num_users++;
        }
    }
    ENetPacket* packet = enet_packet_create(NULL, 1, ENET_PACKET_FLAG_RELIABLE);
    stream_t    stream = {packet->data, packet->dataLength, 0};
    stream_write_u8(&stream, server->protocol.num_users);
    enet_peer_send(server->master.peer, 0, packet);
}

int master_connect(server_t* server, uint16_t port)
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
        ENetPacket* packet = enet_packet_create(NULL,
                                                9 + strlen(server->server_name) + strlen(server->gamemode_name) +
                                                strlen(server->map_name) + 3,
                                                ENET_PACKET_FLAG_RELIABLE);
        stream_t    stream = {packet->data, packet->dataLength, 0};
        stream_write_u8(&stream, 32);
        stream_write_u16(&stream, port);
        stream_write_array(&stream, server->server_name, strlen(server->server_name) + 1);
        stream_write_array(&stream, server->gamemode_name, strlen(server->gamemode_name) + 1);
        stream_write_array(&stream, server->map_name, strlen(server->map_name) + 1);
        enet_peer_send(server->master.peer, 0, packet);
    }
    return 0;
}

void* master_keep_alive(void* p_server)
{
    server_t* server = p_server;
    while (server->running) {
        if (server->master.enable_master_connection == 1) {
            if (time(NULL) - server->master.time_since_last_send >= 1) {
                if (enet_host_service(server->master.client, &server->master.event, 0) < 0) {
                    pthread_mutex_lock(&server_lock);
                    LOG_WARNING("Connection to master server lost. Waiting 30 seconds to reconnect...");
                    sleep(30);
                    master_connect(server, server->port);
                    pthread_mutex_unlock(&server_lock);
                }
                server->master.time_since_last_send = time(NULL);
            }
        }
        sleep(0);
    }
    enet_host_destroy(server->master.client);
    pthread_exit(0);

    return NULL;
}
