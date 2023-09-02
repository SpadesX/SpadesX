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
#include <Server/Gamemodes/GamemodeLua.h>

lua_State* LuaLevel;
int        lua_script = 0;
int ref;

// LUA utils... Should be moved later.
// Set table on top of the stack as read-only.
// It does NOT pop it at the end.
// The table on the top of the stack at the end is NOT the one that was there at the begining.
// It is replaced by another one. So do not store refs to the first one.
static inline int l_read_only(lua_State *L)
{
    lua_settop(L, 0);
    printf("Error: attempted to edit a readonly table.\n");
    return 0;
}
static inline void set_table_as_readonly(lua_State *L)
{
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_newtable(L);
    lua_newtable(L);
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, l_read_only);
    lua_setfield(L, -2, "__newindex");
    lua_pushstring(L, "Read only table. Metatable hidden.");
    lua_setfield(L, -2, "__metatable");
    lua_setmetatable(L, -2);
    luaL_unref(L, LUA_REGISTRYINDEX, ref);
}

uint8_t grenade_gamemode_check(server_t* server, player_t* player, vector3f_t pos)
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
            
            // The player table is readonly. I you want to set something, use a method (not implemented yet) and maybe a fech method to refresh fields.
            set_table_as_readonly(LuaLevel);


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
        return 0; // Handle the error appropriately
    }

    // Push any arguments for the function here
    push_init_api(LuaLevel);
    // Now the api table is on top. Let's make it readonly to avoid misuse.
    set_table_as_readonly(LuaLevel);
    // Call the function with the appropriate number of arguments and return values
    if (lua_pcall(LuaLevel, 1, 0, 0) != LUA_OK) {
        LOG_ERROR("Error calling Lua function: %s\n", lua_tostring(LuaLevel, -1));
        lua_pop(LuaLevel, 2); // Pop the error message and the table
        return 0; // Handle the error appropriately
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
    if (access("level.lua", F_OK) == 0) {
        LuaLevel = luaL_newstate();
        luaL_openlibs(LuaLevel);
        register_spadesx_module(LuaLevel);
        if (luaL_dofile(LuaLevel, "level.lua") != LUA_OK) {
            LOG_ERROR("Error loading Lua file: %s\n", lua_tostring(LuaLevel, -1));
            server->running = 0;
        }
        ref = luaL_ref(LuaLevel, LUA_REGISTRYINDEX);
        lua_script      = 1;

        server->running = _init_lua(server);
    };
}
