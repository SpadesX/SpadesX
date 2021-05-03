#include <stdio.h>
#include "Structs.h"
#include "Queue.h"
#include "Types.h"
#include "Compress.h"
#include "DataStream.h"

void LoadMap(Server* server, const char* path)
{
	STATUS("loading map");

	while (server->map.compressedMap) {
		server->map.compressedMap = Pop(server->map.compressedMap);
	}

	FILE* file = fopen(path, "rb");
	if (!file) {
		perror("file not found");
		exit(EXIT_FAILURE);
	}

	server->map.mapSize = 1024*1024*64; //The size of file wasnt enough memory for it. Lets bump it up to this for now to fix building
	fseek(file, 0, SEEK_SET);

	uint8* buffer = (uint8*) malloc(server->map.mapSize);
	uint8* mapOut = (uint8*) malloc(server->map.mapSize);
	fread(buffer, server->map.mapSize, 1, file);
	fclose(file);
	libvxl_create(&server->map.map, 512, 512, 64, buffer, server->map.mapSize);
	STATUS("compressing map data");

	libvxl_write(&server->map.map, mapOut, &server->map.mapSize);
	server->map.compressedMap = CompressData(mapOut, server->map.mapSize, DEFAULT_COMPRESSOR_CHUNK_SIZE);
	free(buffer);
	free(mapOut);

	Queue* node = server->map.compressedMap;
	server->map.compressedSize = 0;
	while (node) {
		server->map.compressedSize += node->length;
		node = node->next;
	}
}

void SendMapStart(Server* server, uint8 playerID)
{
	STATUS("sending map info");
	ENetPacket* packet = enet_packet_create(NULL, 5, ENET_PACKET_FLAG_RELIABLE);
	DataStream  stream = {packet->data, packet->dataLength, 0};
	WriteByte(&stream, PACKET_TYPE_MAP_START);
	WriteInt(&stream, server->map.compressedSize);
	if (enet_peer_send(server->player[playerID].peer, 0, packet) == 0) {
		server->player[playerID].state = STATE_LOADING_CHUNKS;
		
		// map
		uint8* out = (uint8*) malloc(server->map.mapSize);
		libvxl_write(&server->map.map, out, &server->map.mapSize);
		server->map.compressedMap = CompressData(out, server->map.mapSize, DEFAULT_COMPRESSOR_CHUNK_SIZE);
		server->player[playerID].queues = server->map.compressedMap;
		free(out);
	}
}

void SendMapChunks(Server* server, uint8 playerID)
{
	if (server->player[playerID].queues == NULL) {
		server->player[playerID].state = STATE_JOINING;
		STATUS("loading chunks done");
	} else {
		ENetPacket* packet = enet_packet_create(NULL, server->player[playerID].queues->length + 1, ENET_PACKET_FLAG_RELIABLE);
		DataStream  stream = {packet->data, packet->dataLength, 0};
		WriteByte(&stream, PACKET_TYPE_MAP_CHUNK);
		WriteArray(&stream, server->player[playerID].queues->block, server->player[playerID].queues->length);

		if (enet_peer_send(server->player[playerID].peer, 0, packet) == 0) {
			server->player[playerID].queues = server->player[playerID].queues->next;
		}
	}
}
