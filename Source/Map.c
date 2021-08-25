// Copyright DarkNeutrino 2021
#include "Structs.h"

#include <Compress.h>
#include <DataStream.h>
#include <Queue.h>
#include <Types.h>
#include <libmapvxl/libmapvxl.h>
#include <stdio.h>

int color4iToInt(Color4i color);

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

    server->map.mapSize = 1024 * 1024 * 10;
    fseek(file, 0, SEEK_SET);

    uint8* buffer = (uint8*) malloc(server->map.mapSize);
    uint8* mapOut = (uint8*) malloc(server->map.mapSize);
    fread(buffer, server->map.mapSize, 1, file);
    fclose(file);
    mapvxlLoadVXL(&server->map.map, buffer);
    STATUS("compressing map data");

    mapvxlWriteMap(&server->map.map, mapOut);
    server->map.compressedMap = CompressData(mapOut, server->map.mapSize, DEFAULT_COMPRESSOR_CHUNK_SIZE);
    free(buffer);
    free(mapOut);

    Queue* node                = server->map.compressedMap;
    server->map.compressedSize = 0;
    while (node) {
        server->map.compressedSize += node->length;
        node = node->next;
    }
}

void writeBlockLine(Server* server, uint8 playerID, vec3i* start, vec3i* end)
{
    int size = blockLine(start, end, server->map.resultLine);
    server->player[playerID].blocks -= size;
    for (int i = 0; i < size; i++) {
        mapvxlSetColor(&server->map.map,
                       server->map.resultLine[i].x,
                       server->map.resultLine[i].y,
                       server->map.resultLine[i].z,
                       color4iToInt(server->player[playerID].toolColor));
    }
}
