#include "../Extern/libmapvxl/libmapvxl.h"
#include "Protocol.h"
#include "Structs.h"
#include "Util/Types.h"
#include "Util/Enums.h"
#include "Util/Log.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static void initCTF(Server* server)
{
    memcpy(server->gamemodeName, "ctf", strlen("ctf") + 1);
    // Init CTF
    server->protocol.gameMode.score[0]   = 0;
    server->protocol.gameMode.score[1]   = 0;
    server->protocol.gameMode.scoreLimit = 10;
    server->protocol.gameMode.intelFlags = 0;
    // intel
    server->protocol.gameMode.intel[0]     = SetIntelTentSpawnPoint(server, 0);
    server->protocol.gameMode.intel[1]     = SetIntelTentSpawnPoint(server, 1);
    server->protocol.gameMode.intelHeld[0] = 0;
    server->protocol.gameMode.intelHeld[1] = 0;
    // bases
    server->protocol.gameMode.base[0] = SetIntelTentSpawnPoint(server, 0);
    server->protocol.gameMode.base[1] = SetIntelTentSpawnPoint(server, 1);

    server->protocol.gameMode.base[0].x = floorf(server->protocol.gameMode.base[0].x);
    server->protocol.gameMode.base[0].y = floorf(server->protocol.gameMode.base[0].y);
    server->protocol.gameMode.base[1].x = floorf(server->protocol.gameMode.base[1].x);
    server->protocol.gameMode.base[1].y = floorf(server->protocol.gameMode.base[1].y);
}

static void initTC(Server* server)
{
    memcpy(server->gamemodeName, "tc", strlen("tc") + 1);
    LOG_WARNING("GameMode not supported properly yet");
    server->running = 0;
}

static void initBabel(Server* server)
{
    memcpy(server->gamemodeName, "babel", strlen("babel") + 1);
    // Init CTF
    server->protocol.gameMode.score[0]   = 0;
    server->protocol.gameMode.score[1]   = 0;
    server->protocol.gameMode.scoreLimit = 10;
    server->protocol.gameMode.intelFlags = 0;

    Color4i platformColor;
    platformColor.colorArray[B_CHANNEL] = 255;
    platformColor.colorArray[G_CHANNEL] = 255;
    platformColor.colorArray[R_CHANNEL] = 0;

    for (int x = 206; x <= 306; ++x) {
        for (int y = 240; y <= 272; ++y) {
            mapvxl_set_color(&server->map.map, x, y, 1, platformColor.color);
        }
    }
    // intel
    server->protocol.gameMode.intel[0].z =
    mapvxl_find_top_block(&server->map.map, 255, 255); // We still need highest point of map. While this is 0 for normal
                                                    // map. The platform may not be there in all sizes
    server->protocol.gameMode.intel[0].x   = round((float)server->map.map.size_x / 2);
    server->protocol.gameMode.intel[0].y   = round((float)server->map.map.size_y / 2);
    server->protocol.gameMode.intelHeld[0] = 0;

    server->protocol.gameMode.intel[1].z =
    mapvxl_find_top_block(&server->map.map, 255, 255); // We still need highest point of map. While this is 0 for normal
                                                    // map. The platform may not be there in all sizes
    server->protocol.gameMode.intel[1].x   = round((float)server->map.map.size_x / 2);
    server->protocol.gameMode.intel[1].y   = round((float)server->map.map.size_y / 2);
    server->protocol.gameMode.intelHeld[1] = 0;
    // bases
    server->protocol.gameMode.base[0] = SetIntelTentSpawnPoint(server, 0);
    server->protocol.gameMode.base[1] = SetIntelTentSpawnPoint(server, 1);

    server->protocol.gameMode.base[0].x = floorf(server->protocol.gameMode.base[0].x);
    server->protocol.gameMode.base[0].y = floorf(server->protocol.gameMode.base[0].y);
    server->protocol.gameMode.base[1].x = floorf(server->protocol.gameMode.base[1].x);
    server->protocol.gameMode.base[1].y = floorf(server->protocol.gameMode.base[1].y);
}

static void initArena(Server* server)
{
    memcpy(server->gamemodeName, "arena", strlen("arena") + 1);
    LOG_WARNING("GameMode not supported properly yet");
    server->running = 0;
}

void initGameMode(Server* server, uint8 gamemode)
{
    server->protocol.currentGameMode = gamemode;

    if (server->protocol.currentGameMode == GAME_MODE_CTF) {
        initCTF(server);
    } else if (server->protocol.currentGameMode == GAME_MODE_TC) {
        initTC(server);
    } else if (server->protocol.currentGameMode == GAME_MODE_BABEL) {
        initBabel(server);
    } else if (server->protocol.currentGameMode == GAME_MODE_ARENA) {
        initArena(server);
    } else {
        LOG_ERROR("Unknown GameMode");
        server->running = 0;
    }
}
