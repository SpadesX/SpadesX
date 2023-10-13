#include <Server/Gamemodes/GamemodeLua.h>
#include <Server/Gamemodes/Gamemodes.h>
#include <Server/IntelTent.h>
#include <Server/Server.h>
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
int        ref;

static inline int push_check_func_safely(lua_State* L, const char* func_name)
{
    // Get the spadesx table returned by the script.
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    // Check if the value is a table
    if (!lua_istable(L, -1)) {
        LOG_ERROR("Error: did the GameMode.lua return an instance of require('spadesx')?\n");
        lua_pop(L, 1);

        return 0;
    }
    // Checks that spadesx.checks exist.
    lua_getfield(L, -1, "checks");
    if (!lua_istable(L, -1)) {
        LOG_ERROR("The spadesx.checks is not a table.\n");
        lua_pop(L, 2); // Pop the function, "block" subtable, and the table
        return 0;      // Handle the error appropriately
    }
    // Push the member function onto the stack (replace "functionName" with your function name)
    lua_getfield(L, -1, func_name);
    // Check if the value is a function
    if (!lua_isfunction(L, -1)) {
        LOG_ERROR("The spadesx table does not contain a member checks.%s().\n", func_name);
        lua_pop(L, 3); // Pop the function and the table
        return 0;      // Handle the error appropriately
    }
    // All checks passed.
    // The function is on top of the stack.
    return 1;
}

void gamemode_on_player_join(player_t* player) {
    // Retrieve the table from the Lua registry
    lua_rawgeti(LuaLevel, LUA_REGISTRYINDEX, ref);

    // Check if 'players' is a table
    lua_pushstring(LuaLevel, "players");
    lua_gettable(LuaLevel, -2);
    if (lua_istable(LuaLevel, -1)) {
        // Push the numeric index
        lua_pushinteger(LuaLevel, player->id+1);        
        // Push the value onto the stack
        //lua_pushstring(LuaLevel, "value");
        push_player_api(LuaLevel, player);



        // Set the value at the specified index in the 'players' table
        lua_settable(LuaLevel, -3);
    } else {
        // Handle the case where 'players' is not a table
        printf("Error: 'players' is not a table.\n");
    }

    // Pop the temporary values
    lua_pop(LuaLevel, 3);
}

void gamemode_on_player_left(player_t* player){
    player++;
}

static inline int gamemode_block_check_destruction(player_t* from, uint8_t tool, uint16_t x, uint16_t y, uint16_t z){
    if (push_check_func_safely(LuaLevel, "block_destruction")) {
        lua_pushinteger(LuaLevel, from->id+1);
        lua_pushinteger(LuaLevel, tool);
        push_block(LuaLevel, x, y, z, mapvxl_get_color(&get_server()->s_map.map, x, y, z));

        // Call the function with the appropriate number of arguments and return values
        if (lua_pcall(LuaLevel, 3, 1, 0) != LUA_OK) {
            LOG_ERROR("Error calling Lua function: %s\n", lua_tostring(LuaLevel, -1));
            lua_pop(LuaLevel, 4); // Pop the error message and the table
            return 0;             // Handle the error appropriately
        } else {
            // Check if the returned value is an integer
            if (lua_isinteger(LuaLevel, -1)) {
                lua_Integer result = lua_tointeger(LuaLevel, -1);

                // Pop the table (and any return values if needed)
                lua_pop(LuaLevel, 1);
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

uint8_t gamemode_block_creation_checks(player_t* from, uint16_t x, uint16_t y, uint16_t z)
{
    if (push_check_func_safely(LuaLevel, "block_placement")) {

        lua_pushinteger(LuaLevel, from->id+1);
        push_block(LuaLevel, x, y, z, mapvxl_get_color(&get_server()->s_map.map, x, y, z));

        // Call the function with the appropriate number of arguments and return values
        if (lua_pcall(LuaLevel, 2, 1, 0) != LUA_OK) {
            LOG_ERROR("Error calling Lua function: %s\n", lua_tostring(LuaLevel, -1));
            lua_pop(LuaLevel, 4); // Pop the error message and the table
            return 0;             // Handle the error appropriately
        } else {
            // Check if the returned value is an integer
            if (lua_isinteger(LuaLevel, -1)) {
                lua_Integer result = lua_tointeger(LuaLevel, -1);

                // Pop the table (and any return values if needed)
                lua_pop(LuaLevel, 1);
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

uint8_t gamemode_grenade_destruction_checks(player_t* player, vector3f_t pos)
{
    if (gamemode_block_check_destruction(player,TOOL_GRENADE, pos.x + 1, pos.y + 1, pos.z + 1) &&
        gamemode_block_check_destruction(player, TOOL_GRENADE,pos.x + 1, pos.y + 1, pos.z - 1) &&
        gamemode_block_check_destruction(player, TOOL_GRENADE,pos.x + 1, pos.y + 1, pos.z) &&
        gamemode_block_check_destruction(player, TOOL_GRENADE,pos.x + 1, pos.y - 1, pos.z + 1) &&
        gamemode_block_check_destruction(player, TOOL_GRENADE,pos.x + 1, pos.y - 1, pos.z - 1) &&
        gamemode_block_check_destruction(player, TOOL_GRENADE,pos.x + 1, pos.y - 1, pos.z) &&
        gamemode_block_check_destruction(player,TOOL_GRENADE,pos.x - 1, pos.y + 1, pos.z + 1) &&
        gamemode_block_check_destruction(player, TOOL_GRENADE,pos.x - 1, pos.y + 1, pos.z - 1) &&
        gamemode_block_check_destruction(player, TOOL_GRENADE,pos.x - 1, pos.y + 1, pos.z) &&
        gamemode_block_check_destruction(player, TOOL_GRENADE,pos.x - 1, pos.y - 1, pos.z + 1) &&
        gamemode_block_check_destruction(player, TOOL_GRENADE,pos.x - 1, pos.y - 1, pos.z - 1) &&
        gamemode_block_check_destruction(player, TOOL_GRENADE,pos.x - 1, pos.y - 1, pos.z))
    {
        return 1;
    }
    return 0;
}

uint8_t gamemode_hand_destruction_checks(player_t* from, uint16_t x, uint16_t y, uint16_t z)
{
    return gamemode_block_check_destruction(from, TOOL_SPADE, x, y, z);
}

uint8_t gamemode_gun_destruction_checks(player_t* from, uint16_t x, uint16_t y, uint16_t z)
{
    return gamemode_block_check_destruction(from, TOOL_GUN, x, y, z);
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

    // Get the spadesx table returned by the script.
    lua_rawgeti(LuaLevel, LUA_REGISTRYINDEX, ref);
    // Check if the value is a table
    if (!lua_istable(LuaLevel, -1)) {
        LOG_ERROR("Referenced value is not a table.\n");
        return 0; // Handle the error appropriately
    }
    // Push the member function onto the stack (replace "functionName" with your function name)
    lua_getfield(LuaLevel, -1, "init");
    // Check if the value is a function
    if (!lua_isfunction(LuaLevel, -1)) {
        LOG_ERROR("The spadesx table does not contain a member init().\n");
        lua_pop(LuaLevel, 2); // Pop the function and the table
        return 0;             // Handle the error appropriately
    }

    // Give a table to the init function with needed API.
    push_init_api(LuaLevel);
    // Call the function with the appropriate number of arguments and return values
    if (lua_pcall(LuaLevel, 1, 0, 0) != LUA_OK) {
        LOG_ERROR("Error calling Lua function: %s\n", lua_tostring(LuaLevel, -1));
        lua_pop(LuaLevel, 2); // Pop the error message and the table
        return 0;             // Handle the error appropriately
    }

    // Pop the table (and any return values if needed)
    lua_pop(LuaLevel, 1);

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
    if (access("GameMode.lua", F_OK) == 0) {
        LuaLevel = luaL_newstate();
        luaL_openlibs(LuaLevel);
        register_spadesx_module(LuaLevel);
        if (luaL_dofile(LuaLevel, "GameMode.lua") != LUA_OK) {
            LOG_ERROR("Error loading GameMode.lua file: %s\n", lua_tostring(LuaLevel, -1));
            server->running = 0;
        }
        ref        = luaL_ref(LuaLevel, LUA_REGISTRYINDEX);
        lua_script = 1;

        server->running = _init_lua(server);
    };
}
void gamemode_reset()
{
    luaL_unref(LuaLevel, LUA_REGISTRYINDEX, ref);
    lua_close(LuaLevel);
}
