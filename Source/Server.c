#include "Server.h"

#include "Compress.h"
#include "DataStream.h"

#include <enet/enet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void ServerStart(Server* server,
                 uint16  port,
                 uint32  connections,
                 uint32  channels,
                 uint32  inBandwidth,
                 uint32  outBandwidth)
{
    printf("INFO: initializing ENet\n");

    if (enet_initialize() != 0) {
        fprintf(stderr, "ERROR: failed to initialize ENet\n");
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

    InitCompressor(5);
}

void ServerDestroy(Server* server)
{
    printf("INFO: destroying server\n");

    CloseCompressor();

    enet_host_destroy(server->host);
}

Connection* CurrentConnection = NULL;
Queue*      MapData           = NULL;

void ServerStep(Server* server, int timeout)
{
    ENetPacket* packet = enet_packet_create("ok", 3, ENET_PACKET_FLAG_RELIABLE);

    ENetEvent event;
    while (enet_host_service(server->host, &event, timeout) > 0) {

        if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
            printf("INFO: connection %X terminated\n", event.peer->address.host);
            Connection* connection = event.peer->data;
            connection->state      = CONNECTION_DISCONNECTED;
            connection->peer       = NULL;
            CurrentConnection      = NULL;
        }

        if (event.type == ENET_EVENT_TYPE_CONNECT) {
            printf("INFO: new connection: %X:%u, channel %u, data %u\n",
                   event.peer->address.host,
                   event.peer->address.port,
                   event.channelID,
                   event.data);

            event.peer->data       = server->connections + 13;
            Connection* connection = event.peer->data;

            connection->playerID = 13;
            connection->state    = CONNECTION_STARTING_MAP;
            connection->peer     = event.peer;

            CurrentConnection = connection;
        }

        if (event.type == ENET_EVENT_TYPE_RECEIVE) {
            printf("PACKET: from %X:%u, channel %u, length %zu, code: %u\n",
                   event.peer->address.host,
                   event.peer->address.port,
                   event.channelID,
                   event.packet->dataLength,
                   event.packet->data[0]);

            DataStream stream = {event.packet->data, event.packet->dataLength, 0};

            Connection* connection = event.peer->data;
            if (ReadByte(&stream) == 9) {
                printf("id %u\n", ReadByte(&stream));
                printf("team %u\n", ReadByte(&stream));
                printf("weapon %u\n", ReadByte(&stream));
                printf("held %u\n", ReadByte(&stream));
                printf("kills %u\n", ReadInt(&stream));
                printf("rgb: %X %X %X\n", ReadByte(&stream), ReadByte(&stream), ReadByte(&stream));

                uint32 length = DataLeft(&stream);
                printf("name length: %u\n", length);
                char* name = malloc(length + 1);
                ReadArray(&stream, name, length);
                name[length] = '\0';
                // ReadArray(&stream, name, length);
                printf("name: '%s'\n", name);
                free(name);

                connection->state = CONNECTION_SPAWNING;
            }

            enet_packet_destroy(event.packet);
        }
    }

    Connection* connection = CurrentConnection;
    if (connection == NULL) {
        return;
    }

    if (connection->state == CONNECTION_STARTING_MAP) {
        printf("STATUS: loading map\n");

        if (MapData == NULL) {
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

            while (MapData) {
                MapData = Pop(MapData);
            }

            MapData = CompressData(buffer, size, DEFAULT_COMPRESSOR_CHUNK_SIZE);
            free(buffer);
        }

        Queue* node    = MapData;
        uint32 mapSize = 0;
        while (node) {
            mapSize += node->length;
            node = node->next;
        }

        printf("STATUS: sending map info\n");

        ENetPacket* packet = enet_packet_create(NULL, 5, ENET_PACKET_FLAG_RELIABLE);
        DataStream  stream = {packet->data, packet->dataLength, 0};
        WriteByte(&stream, PACKET_TYPE_MAP_START);
        WriteInt(&stream, mapSize);

        if (enet_peer_send(connection->peer, 0, packet) == 0) {
            connection->state = CONNECTION_LOADING_CHUNKS;
        }
    }

    if (connection->state == CONNECTION_LOADING_CHUNKS) {
        if (MapData == NULL) {
            connection->state = CONNECTION_LOADING_STATE;
            printf("STATUS: loading chunks done\n");
        } else {
            ENetPacket* packet = enet_packet_create(NULL, MapData->length + 1, ENET_PACKET_FLAG_RELIABLE);
            DataStream  stream = {packet->data, packet->dataLength, 0};
            WriteByte(&stream, PACKET_TYPE_MAP_CHUNK);
            WriteArray(&stream, MapData->block, MapData->length);

            printf("sending map data, size: %u\n", MapData->length);

            if (enet_peer_send(connection->peer, 0, packet) == 0) {
                MapData = Pop(MapData);
            }
        }
    }

    if (connection->state == CONNECTION_LOADING_STATE) {
        printf("STATUS: sending state\n");

        ENetPacket* packet = enet_packet_create(NULL, 104, ENET_PACKET_FLAG_RELIABLE);
        DataStream  stream = {packet->data, packet->dataLength, 0};
        WriteByte(&stream, PACKET_TYPE_STATE_DATA);
        WriteByte(&stream, connection->playerID);
        WriteColor3iv(&stream, 0xFF, 0xFF, 0xFF);
        WriteColor3iv(&stream, 0xFF, 0x00, 0x00);
        WriteColor3iv(&stream, 0x00, 0xFF, 0x00);
        WriteArray(&stream, "RED TEAM  ", 10);
        WriteArray(&stream, "GREEN TEAM", 10);
        WriteByte(&stream, GAMEMODE_CTF);

        // if gamemode == GAMEMODE_CTF

        WriteByte(&stream, 9);  // SCORE TEAM 1
        WriteByte(&stream, 1);  // SCORE TEAM 2
        WriteByte(&stream, 10); // SCORE LIMIT
        WriteByte(&stream, 0);  // INTEL FLAGS

        // INTEL 1 IF FLAG & 1 == 0
        WriteFloat(&stream, 0.f); // X
        WriteFloat(&stream, 0.f); // Y
        WriteFloat(&stream, 0.f); // Z

        // INTEL 2 IF FLAG & 2 == 0
        WriteFloat(&stream, 0.f); // X
        WriteFloat(&stream, 0.f); // Y
        WriteFloat(&stream, 0.f); // Z

        // TEAM 1 BASE
        WriteFloat(&stream, 1.f); // X
        WriteFloat(&stream, 1.f); // Y
        WriteFloat(&stream, 0.f); // Z

        // TEAM 2 BASE
        WriteFloat(&stream, 2.f); // X
        WriteFloat(&stream, 4.f); // Y
        WriteFloat(&stream, 0.f); // Z

        if (enet_peer_send(connection->peer, 0, packet) == 0) {
            connection->state = CONNECTION_HOLD;
        }
    }

    if (connection->state == CONNECTION_SPAWNING) {
        // player id 	UByte 	0
        // weapon 	UByte 	0
        // team 	Byte 	0
        // x position 	LE Float 	0
        // y position 	LE Float 	0
        // z position 	LE Float 	0
        // name 	CP437 String 	Deuce
        ENetPacket* packet = enet_packet_create(NULL, 15 + 2, ENET_PACKET_FLAG_RELIABLE);
        DataStream  stream = {packet->data, packet->dataLength, 0};
        WriteByte(&stream, PACKET_TYPE_CREATE_PLAYER);
        WriteByte(&stream, connection->playerID); // ID
        WriteByte(&stream, 0);                    // WEAPON
        WriteByte(&stream, 0);                    // TEAM
        WriteFloat(&stream, 120.f);               // X
        WriteFloat(&stream, 256.f);               // Y
        WriteFloat(&stream, 62.f);                // Z
        WriteArray(&stream, "dotnet", 7);

        if (enet_peer_send(connection->peer, 0, packet) == 0) {
            connection->state = CONNECTION_HOLD;
        }
    }
}
