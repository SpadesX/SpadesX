#include "../Extern/libmapvxl/libmapvxl.h"
#include "Protocol.h"
#include "Structs.h"
#include "Util/Enums.h"
#include "Util/Log.h"
#include "Util/Types.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static void _init_ctf(server_t* server)
{
    memcpy(server->gamemode_name, "ctf", strlen("ctf") + 1);
    // Init CTF
    server->protocol.gamemode.score[0]    = 0;
    server->protocol.gamemode.score[1]    = 0;
    server->protocol.gamemode.score_limit = 10;
    server->protocol.gamemode.intel_flags = 0;
    // intel
    server->protocol.gamemode.intel[0]      = SetIntelTentSpawnPoint(server, 0);
    server->protocol.gamemode.intel[1]      = SetIntelTentSpawnPoint(server, 1);
    server->protocol.gamemode.intel_held[0] = 0;
    server->protocol.gamemode.intel_held[1] = 0;
    // bases
    server->protocol.gamemode.base[0] = SetIntelTentSpawnPoint(server, 0);
    server->protocol.gamemode.base[1] = SetIntelTentSpawnPoint(server, 1);

    server->protocol.gamemode.base[0].x = floorf(server->protocol.gamemode.base[0].x);
    server->protocol.gamemode.base[0].y = floorf(server->protocol.gamemode.base[0].y);
    server->protocol.gamemode.base[1].x = floorf(server->protocol.gamemode.base[1].x);
    server->protocol.gamemode.base[1].y = floorf(server->protocol.gamemode.base[1].y);
}

static void _init_tc(server_t* server)
{
    memcpy(server->gamemode_name, "tc", strlen("tc") + 1);
    LOG_WARNING("GameMode not supported properly yet");
    server->running = 0;
}

static void _init_babel(server_t* server)
{
    memcpy(server->gamemode_name, "babel", strlen("babel") + 1);
    // Init CTF
    server->protocol.gamemode.score[0]    = 0;
    server->protocol.gamemode.score[1]    = 0;
    server->protocol.gamemode.score_limit = 10;
    server->protocol.gamemode.intel_flags = 0;

    color_t platformColor;
    platformColor.raw = 0xFF00FFFF;

    for (int x = 206; x <= 306; ++x) {
        for (int y = 240; y <= 272; ++y) {
            mapvxl_set_color(&server->s_map.map, x, y, 1, platformColor.raw);
        }
    }
    // intel
    server->protocol.gamemode.intel[0].z = mapvxl_find_top_block(&server->s_map.map, 255, 255); // We still need highest point of map. While this is 0 for normal
                                                                                                // map. The platform may not be there in all sizes
    server->protocol.gamemode.intel[0].x    = round((float)server->s_map.map.size_x / 2);
    server->protocol.gamemode.intel[0].y    = round((float)server->s_map.map.size_y / 2);
    server->protocol.gamemode.intel_held[0] = 0;

    server->protocol.gamemode.intel[1].z = mapvxl_find_top_block(&server->s_map.map, 255, 255); // We still need highest point of map. While this is 0 for normal
                                                                                                // map. The platform may not be there in all sizes
    server->protocol.gamemode.intel[1].x    = round((float)server->s_map.map.size_x / 2);
    server->protocol.gamemode.intel[1].y    = round((float)server->s_map.map.size_y / 2);
    server->protocol.gamemode.intel_held[1] = 0;
    // bases
    server->protocol.gamemode.base[0] = SetIntelTentSpawnPoint(server, 0);
    server->protocol.gamemode.base[1] = SetIntelTentSpawnPoint(server, 1);

    server->protocol.gamemode.base[0].x = floorf(server->protocol.gamemode.base[0].x);
    server->protocol.gamemode.base[0].y = floorf(server->protocol.gamemode.base[0].y);
    server->protocol.gamemode.base[1].x = floorf(server->protocol.gamemode.base[1].x);
    server->protocol.gamemode.base[1].y = floorf(server->protocol.gamemode.base[1].y);
}

static void _init_arena(server_t* server)
{
    memcpy(server->gamemode_name, "arena", strlen("arena") + 1);
    LOG_WARNING("GameMode not supported properly yet");
    server->running = 0;
}

void gamemode_init(server_t* server, uint8_t gamemode)
{
    server->protocol.current_gamemode = gamemode;

    if (server->protocol.current_gamemode == GAME_MODE_CTF) {
        _init_ctf(server);
    } else if (server->protocol.current_gamemode == GAME_MODE_TC) {
        _init_tc(server);
    } else if (server->protocol.current_gamemode == GAME_MODE_BABEL) {
        _init_babel(server);
    } else if (server->protocol.current_gamemode == GAME_MODE_ARENA) {
        _init_arena(server);
    } else {
        LOG_ERROR("Unknown GameMode");
        server->running = 0;
    }
}
