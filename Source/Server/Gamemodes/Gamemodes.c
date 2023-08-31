#include <Server/Gamemodes/Gamemodes.h>
#include <Server/IntelTent.h>
#include <Server/Structs/ServerStruct.h>
#include <Util/Enums.h>
#include <Util/Log.h>
#include <Util/Types.h>
#include <lauxlib.h>
#include <libmapvxl/libmapvxl.h>
#include <lua.h>
#include <lualib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

lua_State* LuaLevel;
int        lua_script = 0;

// Methods to be used in the init function.

int lua_init_find_top_block(lua_State* L)
{
    server_t* server = get_server();
    // Check if two arguments (x and y) are provided
    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);

    // Perform your desired functionality using x and y
    int result = (int) mapvxl_find_top_block(&server->s_map.map, x, y);
    // Push the result onto the Lua stack
    lua_pushinteger(L, result);

    // Return the number of return values (1 in this case)
    return 1;
}
// To be used in the init API. Does not trigger any notification sent to players.
int lua_init_add_colored_block(lua_State* L)
{
    server_t* server = get_server();
    // Check if four arguments (x, y, z, color) are provided
    int      x     = luaL_checkinteger(L, 1);
    int      y     = luaL_checkinteger(L, 2);
    int      z     = luaL_checkinteger(L, 3);
    uint32_t color = (uint32_t) luaL_checkinteger(L, 4);

    color_t platformColor;
    platformColor.raw = color;
    mapvxl_set_color(&server->s_map.map, x, y, z, platformColor.raw);
    return 0;
}

// To be used in the init API. Does not trigger any notification sent to players.
int lua_init_set_intel_position(lua_State* L)
{
    // Check if four arguments (x, y, z, team) are provided
    int x    = luaL_checkinteger(L, 1);
    int y    = luaL_checkinteger(L, 2);
    int z    = luaL_checkinteger(L, 3);
    int team = luaL_checkinteger(L, 4);
    // Check if the team value is within the valid range of 0-1
    if (team < 0 || team > 1) {
        return luaL_error(L, "Invalid team value: %d (should be between 0 and 1)", team);
    }

    server_t* server = get_server();
    // As we are in the init context... Maybe the flag cannot be held at the beginning?
    // Or maybe later so we can implement a gamemode where tent is in the ennemy spawnpoint and a random player get the
    // flag?
    server->protocol.gamemode.intel_held[0] = 0;
    server->protocol.gamemode.intel[team].x = x;
    server->protocol.gamemode.intel[team].x = y;
    server->protocol.gamemode.intel[team].x = z;
    return 0;
}
// To be used in the init API. Does not trigger any notification sent to players.
int lua_init_set_base_position(lua_State* L)
{
    // Check if four arguments (x, y, z, team) are provided
    int x    = luaL_checkinteger(L, 1);
    int y    = luaL_checkinteger(L, 2);
    int z    = luaL_checkinteger(L, 3);
    int team = luaL_checkinteger(L, 4);
    // Check if the team value is within the valid range of 0-1
    if (team < 0 || team > 1) {
        return luaL_error(L, "Invalid team value: %d (should be between 0 and 1)", team);
    }

    server_t* server = get_server();

    server->protocol.gamemode.base[team].x = x;
    server->protocol.gamemode.base[team].x = y;
    server->protocol.gamemode.base[team].x = z;
    return 0;
}

uint8_t grenadeGamemodeCheck(server_t* server, player_t* player, vector3f_t pos)
{
    if (gamemode_block_checks(server, player, pos.x + 1, pos.y + 1, pos.z + 1) &&
        gamemode_block_checks(server, player, pos.x + 1, pos.y + 1, pos.z - 1) &&
        gamemode_block_checks(server, player, pos.x + 1, pos.y + 1, pos.z) &&
        gamemode_block_checks(server, player, pos.x + 1, pos.y - 1, pos.z + 1) &&
        gamemode_block_checks(server, player, pos.x + 1, pos.y - 1, pos.z - 1) &&
        gamemode_block_checks(server, player, pos.x + 1, pos.y - 1, pos.z) &&
        gamemode_block_checks(server, player, pos.x - 1, pos.y + 1, pos.z + 1) &&
        gamemode_block_checks(server, player, pos.x - 1, pos.y + 1, pos.z - 1) &&
        gamemode_block_checks(server, player, pos.x - 1, pos.y + 1, pos.z) &&
        gamemode_block_checks(server, player, pos.x - 1, pos.y - 1, pos.z + 1) &&
        gamemode_block_checks(server, player, pos.x - 1, pos.y - 1, pos.z - 1) &&
        gamemode_block_checks(server, player, pos.x - 1, pos.y - 1, pos.z))
    {
        return 1;
    }
    return 0;
}

uint8_t gamemode_block_checks(server_t* server, player_t* player, int x, int y, int z)
{
    if (lua_script == 1) {
        lua_getglobal(LuaLevel, "gamemode_block_checks");
        if (lua_isfunction(LuaLevel, -1)) {
            // Push the player table first
            lua_newtable(LuaLevel);
            // Add player name to the table
            lua_pushstring(LuaLevel, "name");
            lua_pushstring(LuaLevel, player->name);
            lua_settable(LuaLevel, -3);

            // Add player id to the table
            lua_pushstring(LuaLevel, "id");
            lua_pushinteger(LuaLevel, player->id);
            lua_settable(LuaLevel, -3);

            // Add player id to the table
            lua_pushstring(LuaLevel, "team");
            lua_pushinteger(LuaLevel, player->team);
            lua_settable(LuaLevel, -3);


            lua_pushstring(LuaLevel, "position");
            lua_newtable(LuaLevel); // Create the sub-table
            lua_pushstring(LuaLevel, "x");
            lua_pushinteger(LuaLevel, player->movement.position.x); // Replace with the actual value
            lua_settable(LuaLevel, -3);
            lua_pushstring(LuaLevel, "y");
            lua_pushinteger(LuaLevel, player->movement.position.y); // Replace with the actual value
            lua_settable(LuaLevel, -3);
            lua_settable(LuaLevel, -3);
            
            // Add player team to the table


            lua_pushinteger(LuaLevel, x); // Argument x
            lua_pushinteger(LuaLevel, y); // Argument y
            lua_pushinteger(LuaLevel, z); // Argument z
            // Call the function
            if (lua_pcall(LuaLevel, 4, 1, 0) != LUA_OK) {
                LOG_ERROR("Error calling Lua function: %s\n", lua_tostring(LuaLevel, -1));
            } else {
                // Check if the returned value is an integer
                if (lua_isinteger(LuaLevel, -1)) {
                    lua_Integer result = lua_tointeger(LuaLevel, -1);
                    return ((uint8_t) result);
                } else {
                    int returnType = lua_type(LuaLevel, -1);
                    LOG_ERROR("Type of the returned value: %s\n", lua_typename(LuaLevel, returnType));
                    LOG_ERROR("Returned value is not an integer.\n");
                }
            }
            LOG_ERROR("Error: call for gamemode_block_checks failed.");
        }
        return 1;
    }

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

static int _init_lua(server_t* server)
{

    // Server initialisation. Might be overriden by lua.

    memcpy(server->gamemode_name, "Lua", strlen("Lua") + 1);
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

    // I want to call the gamemode_init function from the lua file.
    // gamemode_init() will receive as an argument a table containing the api for level initialisation.
    // Achtung: creating blocks won't send an event to players. Do not attempt to use it outside the init function.

    // So First, let's push the api table.
    lua_newtable(LuaLevel);

    // Add C functions to the api table
    lua_pushcfunction(LuaLevel, lua_init_find_top_block);
    lua_setfield(LuaLevel, -2, "find_top_block");

    lua_pushcfunction(LuaLevel, lua_init_add_colored_block);
    lua_setfield(LuaLevel, -2, "add_colored_block");

    lua_pushcfunction(LuaLevel, lua_init_set_intel_position);
    lua_setfield(LuaLevel, -2, "set_intel_position");

    lua_pushcfunction(LuaLevel, lua_init_set_base_position);
    lua_setfield(LuaLevel, -2, "set_base_position");

    // The call gamemode_init(api) function:
    lua_getglobal(LuaLevel, "gamemode_init");
    lua_pushvalue(LuaLevel, -2); // Push the table onto the stack as an argument
    lua_call(LuaLevel, 1, 0);    // Call the init function

    memcpy(server->gamemode_name, "lua", strlen("lua") + 1);

    LOG_WARNING("Running Lua Mode");
    return 1;
}

void gamemode_init(server_t* server, uint8_t gamemode)
{
    server->protocol.current_gamemode = gamemode;

    // First init the gamemode in the config file.
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
    // If a lua file called [level.lua] is provided, it might override some of the functions.
    if (access("level.lua", F_OK) == 0) {
        LuaLevel = luaL_newstate();
        luaL_openlibs(LuaLevel);
        if (luaL_dofile(LuaLevel, "level.lua") != LUA_OK) {
            LOG_ERROR("Error loading Lua file: %s\n", lua_tostring(LuaLevel, -1));
            server->running = 0;
        }
        lua_script      = 1;
        server->running = _init_lua(server);
    };
}
