#include "Enums.h"
#include "Protocol.h"
#include "Structs.h"
#include "Types.h"
#include "libmapvxl.h"

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
    printf("GameMode not supported properly yet\n");
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
    platformColor.colorArray[B_CHANNEL] = 111;
    platformColor.colorArray[G_CHANNEL] = 111;
    platformColor.colorArray[R_CHANNEL] = 111;

    for (int x = 206; x <= 306; ++x) {
        for (int y = 240; y <= 272; ++y) {
            mapvxlSetColor(&server->map.map, x, y, 1, platformColor.color);
        }
    }
    // intel
    server->protocol.gameMode.intel[0].z =
    mapvxlFindTopBlock(&server->map.map, 255, 255); // We still need highest point of map. While this is 0 for normal
                                                    // map. The platform may not be there in all sizes
    server->protocol.gameMode.intel[0].x   = MAP_MAX_X / 2;
    server->protocol.gameMode.intel[0].y   = MAP_MAX_Y / 2;
    server->protocol.gameMode.intelHeld[0] = 0;

    server->protocol.gameMode.intel[1].z =
    mapvxlFindTopBlock(&server->map.map, 255, 255); // We still need highest point of map. While this is 0 for normal
                                                    // map. The platform may not be there in all sizes
    server->protocol.gameMode.intel[1].x   = MAP_MAX_X / 2;
    server->protocol.gameMode.intel[1].y   = MAP_MAX_Y / 2;
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
    printf("GameMode not supported properly yet\n");
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
        printf("Unknown GameMode\n");
        server->running = 0;
    }
}
