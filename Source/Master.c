//Copyright DarkNeutrino 2021
#include <stdio.h>
#include <DataStream.h>
#include <time.h>

#include "Structs.h"
#include "Types.h"

void updateMaster(Server* server) {
	server->protocol.countOfUsers = 0;
	for (int i = 0; i < 32; i++) {
		if (server->player[i].state == STATE_READY) {
			server->protocol.countOfUsers++;
		}
	}
	ENetPacket* packet = enet_packet_create(NULL, 1, ENET_PACKET_FLAG_RELIABLE);
	DataStream  stream = {packet->data, packet->dataLength, 0};
	WriteByte(&stream, server->protocol.countOfUsers);
	enet_peer_send(server->master.peer, 0, packet);
}

void* keepMasterAlive(void* serverVoid) {
	Server *server = serverVoid;
	if (server->master.enableMasterConnection == 1) {
		if (time(NULL) - server->master.timeSinceLastSend >= 5) {
		enet_host_service(server->master.client, &server->master.event, 0);
		server->master.timeSinceLastSend = time(NULL);
		}
	}
}

int ConnectMaster(Server* server, uint16 port) {
    server->master.client = enet_host_create(NULL, 1, 1, 0, 0);

    enet_host_compress_with_range_coder(server->master.client);

    if (server->master.client == NULL) {
        fprintf(stderr, "An error occurred while trying to create an ENet client host!\n");
        return EXIT_FAILURE;
    }

    ENetAddress address;

    enet_address_set_host(&address, "67.205.183.163");
    address.port = 32886;

	STATUS("Connecting to master server");

    server->master.peer = enet_host_connect(server->master.client, &address, 2, 31);
    if (server->master.peer == NULL) {
        fprintf(stderr, "ERROR: failed to create client\n");
        return EXIT_FAILURE;
    }

    ENetEvent event;
    while (enet_host_service(server->master.client, &event, 2000) > 0) {
    				STATUS("Connection success");
				ENetPacket* packet = enet_packet_create(NULL, 61, ENET_PACKET_FLAG_RELIABLE);
				DataStream  stream = {packet->data, packet->dataLength, 0};
				WriteByte(&stream, 32);
				WriteShort(&stream, port);
				WriteArray(&stream, "SpadesX Server", 15);
				WriteArray(&stream, "ctf", 4);
				WriteArray(&stream, "hallway", 8);
				enet_peer_send(server->master.peer, 0, packet);
	}
	return 0;
}