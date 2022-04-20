// Copyright DarkNeutrino 2021
#include "../Extern/libmapvxl/libmapvxl.h"
#include "Structs.h"
#include "Util/Compress.h"
#include "Util/DataStream.h"
#include "Util/Queue.h"
#include "Util/Types.h"

#include <stdio.h>

void LoadMap(Server* server, const char* path)
{
    LOG_STATUS("Loading map");

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

    uint8* buffer = (uint8*) calloc(server->map.mapSize, sizeof(uint8));

    // The biggest possible VXL size given the XYZ size
    uint8* mapOut = (uint8*) calloc(MAP_MAX_X * MAP_MAX_Y * (MAP_MAX_Z / 2), sizeof(uint8));

    if (fread(buffer, server->map.mapSize, 1, file) < server->map.mapSize) {
        LOG_STATUS("Finished loading map");
    }
    fclose(file);

    mapvxlLoadVXL(&server->map.map, buffer);
    free(buffer);
    LOG_STATUS("Compressing map data");

    // Write map to mapOut
    server->map.mapSize = mapvxlWriteMap(&server->map.map, mapOut);
    // Resize the map to the exact VXL memory size for given XYZ coordinate size
    mapOut = (uint8*) realloc(mapOut, server->map.mapSize);

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
