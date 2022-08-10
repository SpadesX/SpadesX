// Copyright DarkNeutrino 2021
#include "../Extern/libmapvxl/libmapvxl.h"
#include "Structs.h"
#include "Util/Compress.h"
#include "Util/DataStream.h"
#include "Util/Queue.h"
#include "Util/Types.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

uint8 LoadMap(Server* server, const char* path, int mapSize[3])
{
    LOG_STATUS("Loading map");

    if (server->map.map.blocks != NULL) {
        mapvxlFree(&server->map.map);
    }

    while (server->map.compressedMap) {
        server->map.compressedMap = Pop(server->map.compressedMap);
    }

    FILE* file = fopen(path, "rb");
    if (!file) {
        LOG_ERROR("Unable to open map at path %s with error: %s", path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    fseek(file, 0L, SEEK_END);
    server->map.mapSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    //Create the array for map with the defined sizes
    mapvxlCreate(&server->map.map, mapSize[0], mapSize[1], mapSize[2]);

    size_t maxMapSize = server->map.map.MAP_X_MAX * server->map.map.MAP_Y_MAX * (server->map.map.MAP_Z_MAX / 2) * 8;

    if (server->map.mapSize > maxMapSize) {
        fclose(file);
        LOG_ERROR("Map file %s.vxl is larger then maximum VXL size of X: %d, Y: %d, Z: %d. Please set the correct map "
                  "size in libmapvxl",
                  server->mapName,
                  server->map.map.MAP_X_MAX,
                  server->map.map.MAP_Y_MAX,
                  server->map.map.MAP_Z_MAX);
        mapvxlFree(&server->map.map);
        server->running = 0;
        return 0;
    }

    uint8* buffer = (uint8*) calloc(server->map.mapSize, sizeof(uint8));

    // The biggest possible VXL size given the XYZ size
    uint8* mapOut = (uint8*) calloc(server->map.map.MAP_X_MAX * server->map.map.MAP_Y_MAX * (server->map.map.MAP_Z_MAX / 2), sizeof(uint8));

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
    uint8* oldMapOut;
    oldMapOut = (uint8*) realloc(mapOut, server->map.mapSize);
    if (!oldMapOut) {
        free(mapOut);
        return 0;
    }
    mapOut = oldMapOut;

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
    return 1;
}
