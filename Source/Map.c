// Copyright DarkNeutrino 2021
#include "Structs.h"

#include <Compress.h>
#include <DataStream.h>
#include <Queue.h>
#include <Types.h>
#include <libmapvxl/libmapvxl.h>
#include <stdio.h>

void LoadMap(Server* server, const char* path)
{
    STATUS("Loading map");

    while (server->map.compressedMap) {
        server->map.compressedMap = Pop(server->map.compressedMap);
    }

    FILE* file = fopen(path, "rb");
    if (!file) {
        perror("MAP NOT FOUND");
        exit(EXIT_FAILURE);
    }

    fseek(file, 0L, SEEK_END);
    server->map.mapSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    uint8* buffer = (uint8*) malloc(server->map.mapSize);
    uint8* mapOut = (uint8*) malloc(server->map.mapSize);
    if (fread(buffer, server->map.mapSize, 1, file) < server->map.mapSize) {
        STATUS("Finished loading map");
    }
    fclose(file);
    mapvxlLoadVXL(&server->map.map, buffer);
    free(buffer);
    STATUS("Compressing map data");

    mapvxlWriteMap(&server->map.map, mapOut);
    
    server->map.compressedMap = CompressData(mapOut, server->map.mapSize, DEFAULT_COMPRESSOR_CHUNK_SIZE);
    free(mapOut);

    Queue* node                = server->map.compressedMap;
    server->map.compressedSize = 0;
    while (node) {
        server->map.compressedSize += node->length;
        node = node->next;
    }
    while (server->map.compressedMap) {
        server->map.compressedMap = Pop(server->map.compressedMap);
    }
}
