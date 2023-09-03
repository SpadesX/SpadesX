#include <Server/Gamemodes/GamemodeLua.h>
#include <Server/Server.h>



static int lua_init_find_top_block(lua_State* L)
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
static int lua_init_add_colored_block(lua_State* L)
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
static int lua_init_set_intel_position(lua_State* L)
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
static int lua_init_set_base_position(lua_State* L)
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

// To be used in the init API. Does not trigger any notification sent to players.
static int lua_init_set_team_spawn(lua_State* L)
{
    // Check if 7 arguments (team, x1, y1, z1, x2, y2, z2) are provided
    int team = luaL_checkinteger(L, 4);
    int x1    = luaL_checkinteger(L, 1);
    int y1    = luaL_checkinteger(L, 2);
    int z1    = luaL_checkinteger(L, 3);
    int x2    = luaL_checkinteger(L, 1);
    int y2    = luaL_checkinteger(L, 2);
    int z2    = luaL_checkinteger(L, 3);
    // Check if the team value is within the valid range of 0-1
    if (team < 0 || team > 1) {
        return luaL_error(L, "Invalid team value: %d (should be between 0 and 1)", team);
    }

    server_t* server = get_server();
    server->protocol.spawns[1].from.x = x1;
    server->protocol.spawns[1].from.y = y1;
    server->protocol.spawns[1].from.z = z1;
    server->protocol.spawns[1].to.x   = x2;
    server->protocol.spawns[1].to.y   = y2;
    server->protocol.spawns[1].to.z   = z2;
    return 0;
}


void push_init_api(lua_State * L){
    // I want to call the gamemode_init function from the lua file.
    // gamemode_init() will receive as an argument a table containing the api for level initialisation.
    // Achtung: creating blocks won't send an event to players. Do not attempt to use it outside the init function.

    // So First, let's push the api table.
    lua_newtable(L);

    // Add C functions to the api table
    lua_pushcfunction(L, lua_init_find_top_block);
    lua_setfield(L, -2, "find_top_block");

    lua_pushcfunction(L, lua_init_add_colored_block);
    lua_setfield(L, -2, "add_colored_block");

    lua_pushcfunction(L, lua_init_set_intel_position);
    lua_setfield(L, -2, "set_intel_position");

    lua_pushcfunction(L, lua_init_set_base_position);
    lua_setfield(L, -2, "set_base_position");

    lua_pushcfunction(L, lua_init_set_team_spawn);
    lua_setfield(L, -2, "set_team_spawn");
}