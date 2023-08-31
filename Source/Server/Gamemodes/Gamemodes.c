#include <Server/Gamemodes/Gamemodes.h>
#include <Server/IntelTent.h>
#include <Server/Structs/ServerStruct.h>
#include <Util/Enums.h>
#include <Util/Log.h>
#include <Util/Types.h>
#include <libmapvxl/libmapvxl.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

uint8_t grenadeGamemodeCheck(server_t* server, player_t* player, vector3f_t pos)
{
    if (gamemode_block_checks(server,player, pos.x + 1, pos.y + 1, pos.z + 1) &&
        gamemode_block_checks(server,player, pos.x + 1, pos.y + 1, pos.z - 1) &&
        gamemode_block_checks(server,player, pos.x + 1, pos.y + 1, pos.z) &&
        gamemode_block_checks(server,player, pos.x + 1, pos.y - 1, pos.z + 1) &&
        gamemode_block_checks(server,player, pos.x + 1, pos.y - 1, pos.z - 1) &&
        gamemode_block_checks(server,player, pos.x + 1, pos.y - 1, pos.z) &&
        gamemode_block_checks(server,player, pos.x - 1, pos.y + 1, pos.z + 1) &&
        gamemode_block_checks(server,player, pos.x - 1, pos.y + 1, pos.z - 1) &&
        gamemode_block_checks(server,player, pos.x - 1, pos.y + 1, pos.z) &&
        gamemode_block_checks(server,player, pos.x - 1, pos.y - 1, pos.z + 1) &&
        gamemode_block_checks(server,player, pos.x - 1, pos.y - 1, pos.z - 1) &&
        gamemode_block_checks(server,player, pos.x - 1, pos.y - 1, pos.z))
    {
        return 1;
    }
    return 0;
}

uint8_t gamemode_block_checks(server_t* server, player_t* player, int x, int y, int z)
{
    if (((((x >= 206 && x <= 306) && (y >= 240 && y <= 272) && (z == 2 || z == 0)) ||
          ((x >= 205 && x <= 307) && (y >= 239 && y <= 273) && (z == 1))) &&
         server->protocol.current_gamemode == GAME_MODE_BABEL))
    {
        return 0;
    }
    return 1;
}

static void _init_ctf(server_t* server)
{
    memcpy(server->gamemode_name, "ctf", strlen("ctf") + 1);
    // Init CTF
    server->protocol.gamemode.score[0]    = 0;
    server->protocol.gamemode.score[1]    = 0;
    server->protocol.gamemode.intel_flags = 0;
    // intel
    server->protocol.gamemode.intel[0]      = set_intel_tent_spawn_point(server, 0);
    server->protocol.gamemode.intel[1]      = set_intel_tent_spawn_point(server, 1);
    server->protocol.gamemode.intel_held[0] = 0;
    server->protocol.gamemode.intel_held[1] = 0;
    // bases
    server->protocol.gamemode.base[0] = set_intel_tent_spawn_point(server, 0);
    server->protocol.gamemode.base[1] = set_intel_tent_spawn_point(server, 1);

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
    server->protocol.gamemode.intel_flags = 0;

    color_t platformColor;
    platformColor.raw = 0xFF00FFFF;

    for (int x = 206; x <= 306; ++x) {
        for (int y = 240; y <= 272; ++y) {
            mapvxl_set_color(&server->s_map.map, x, y, 1, platformColor.raw);
        }
    }
    // intel
    server->protocol.gamemode.intel[0].z =
    mapvxl_find_top_block(&server->s_map.map, 255, 255); // We still need highest point of map. While this is 0 for
                                                         // normal map. The platform may not be there in all sizes
    server->protocol.gamemode.intel[0].x    = round((float) server->s_map.map.size_x / 2);
    server->protocol.gamemode.intel[0].y    = round((float) server->s_map.map.size_y / 2);
    server->protocol.gamemode.intel_held[0] = 0;

    server->protocol.gamemode.intel[1].z =
    mapvxl_find_top_block(&server->s_map.map, 255, 255); // We still need highest point of map. While this is 0 for
                                                         // normal map. The platform may not be there in all sizes
    server->protocol.gamemode.intel[1].x    = round((float) server->s_map.map.size_x / 2);
    server->protocol.gamemode.intel[1].y    = round((float) server->s_map.map.size_y / 2);
    server->protocol.gamemode.intel_held[1] = 0;
    // bases
    server->protocol.gamemode.base[0] = set_intel_tent_spawn_point(server, 0);
    server->protocol.gamemode.base[1] = set_intel_tent_spawn_point(server, 1);

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
