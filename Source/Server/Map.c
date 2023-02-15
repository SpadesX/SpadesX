// Copyright DarkNeutrino 2021
#include <Server/Structs/ServerStruct.h>
#include <Util/Compress.h>
#include <Util/DataStream.h>
#include <Util/Log.h>
#include <Util/Queue.h>
#include <Util/Types.h>
#include <Util/Utlist.h>
#include <errno.h>
#include <libmapvxl/libmapvxl.h>
#include <stdio.h>
#include <stdlib.h>

uint8_t map_load(server_t* server, const char* path, int map_size[3])
{
    LOG_STATUS("Loading map");

    if (server->s_map.map.blocks != NULL) {
        mapvxl_free(&server->s_map.map);
    }

    FILE* file = fopen(path, "rb");
    if (!file) {
        LOG_ERROR("Unable to open map at path %s with error: %s", path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    fseek(file, 0L, SEEK_END);
    server->s_map.map_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Create the array for map with the defined sizes
    mapvxl_create(&server->s_map.map, map_size[0], map_size[1], map_size[2]);

    size_t maxMapSize = server->s_map.map.size_x * server->s_map.map.size_y * (server->s_map.map.size_z / 2) * 8;

    if (server->s_map.map_size > maxMapSize) {
        fclose(file);
        LOG_ERROR("Map file %s.vxl is larger then maximum VXL size of X: %d, Y: %d, Z: %d. Please set the correct map "
                  "size in libmapvxl",
                  server->map_name,
                  server->s_map.map.size_x,
                  server->s_map.map.size_y,
                  server->s_map.map.size_z);
        mapvxl_free(&server->s_map.map);
        server->running = 0;
        return 0;
    }

    uint8_t* buffer = (uint8_t*) calloc(server->s_map.map_size, sizeof(uint8_t));

    if (fread(buffer, server->s_map.map_size, 1, file) < server->s_map.map_size) {
        LOG_STATUS("Finished loading map");
    }
    fclose(file);

    LOG_STATUS("Transforming map from VXL");
    mapvxl_read(&server->s_map.map, buffer);
    LOG_STATUS("Finished transforming map");

    free(buffer);
    return 1;
}
