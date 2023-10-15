#include <Server/Block.h>
#include <Server/Gamemodes/GamemodeLua.h>
#include <Server/Packets/Packets.h>
#include <Server/Server.h>
#include <Util/Notice.h>

// Send a notice to the player.
// First argument is the player's table.
// Second is the message.
static int send_notice(lua_State* L)
{

    // Check the number of arguments
    int numArgs = lua_gettop(L);
    if (numArgs < 2) {
        luaL_error(L,
                   "Insufficient arguments. Usage: player:send_notice(notice) or  player.send_notice(player,notice)");
        return 0;
    }

    // Retrieve the player table from the first argument
    luaL_checktype(L, 1, LUA_TTABLE);
    // Assuming the player table has an "address" field, retrieve the player's C struct's address
    lua_getfield(L, 1, "address");
    // Check if the value is a userdata
    player_t** playerUserData = (player_t**) lua_touserdata(L, -1);
    player_t*  player;
    player = *playerUserData;

    const char* message = luaL_checkstring(L, 2);

    send_server_notice(player, 0, message);
    return 0; // Return the number of values pushed onto the Lua stack (in this case, none)
}

// Restock the player.
static int restock(lua_State* L)
{
    server_t* server = get_server();

    // Check the number of arguments
    int numArgs = lua_gettop(L);
    if (numArgs < 1) {
        luaL_error(L, "Insufficient arguments. Usage: player:restock() or  player.restock(player)");
        return 0;
    }

    // Retrieve the player table from the first argument
    luaL_checktype(L, 1, LUA_TTABLE);

    // Assuming the player table has an "addess" field, retrieve the player's C struct's address
    lua_getfield(L, 1, "address");
    // Check if the value is a userdata
    player_t** playerUserData = (player_t**) lua_touserdata(L, -1);
    player_t*  player;
    player = *playerUserData;

    player->blocks = 50;
    send_restock(server, player);
    return 0;
}

// C function for the Lua create_block function
static int create_block_function(lua_State* L)
{
    // Check the number of arguments
    int numArgs = lua_gettop(L);
    if (numArgs < 4) {
        luaL_error(
        L, "Insufficient arguments. Usage: player:create_block(x, y, z) or  player.create_block(player, x, y, z)");
        return 0;
    }

    // Retrieve the player table from the first argument
    luaL_checktype(L, 1, LUA_TTABLE);
    // Assuming the player table has an "addess" field, retrieve the player's C struct's address
    lua_getfield(L, 1, "address");
    // Check if the value is a userdata
    player_t** playerUserData = (player_t**) lua_touserdata(L, -1);
    player_t*  player;
    player = *playerUserData;

    // Retrieve x, y, and z arguments
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    int z = luaL_checkinteger(L, 4);

    server_t* server = get_server();
    mapvxl_set_color(&server->s_map.map, x, y, z, player->tool_color.raw);
    moveIntelAndTentUp(server);
    send_block_action(server, player, BLOCKACTION_BUILD, x, y, z);
    send_block_action_to_player(server, player, player, BLOCKACTION_BUILD, x, y, z);
    player->blocks--;
    return 0;
}

static int get_team(lua_State* L) {
    // Check the number of arguments
    int numArgs = lua_gettop(L);
    if (numArgs < 1) {
        luaL_error(L, "Insufficient arguments. Usage: player:get_team() or  player.get_team(player)");
        return 0;
    }

    // Retrieve the player table from the first argument
    luaL_checktype(L, 1, LUA_TTABLE);
    // Assuming the player table has an "address" field, retrieve the player's C struct's address
    lua_getfield(L, 1, "address");
    // Check if the value is a userdata
    player_t** playerUserData = (player_t**) lua_touserdata(L, -1);
    player_t* player = *playerUserData;

    // Create a the returned table containing color and id.
    lua_newtable(L);
    lua_pushstring(L, "color");
    lua_pushinteger(L, get_server()->protocol.color_team[player->team].raw);
    lua_settable(L, -3);
    lua_pushstring(L, "id");
    lua_pushinteger(L, player->team);
    lua_settable(L, -3);

    return 1;
}

static int get_position(lua_State* L) {
    // Check the number of arguments
    int numArgs = lua_gettop(L);
    if (numArgs < 1) {
        luaL_error(L, "Insufficient arguments. Usage: player:get_position() or  player.get_position(player)");
        return 0;
    }

    // Retrieve the player table from the first argument
    luaL_checktype(L, 1, LUA_TTABLE);
    // Assuming the player table has an "address" field, retrieve the player's C struct's address
    lua_getfield(L, 1, "address");
    // Check if the value is a userdata
    player_t** playerUserData = (player_t**) lua_touserdata(L, -1);
    player_t* player = *playerUserData;

    /* Return the position of the player. */
    lua_newtable(L);
    lua_pushstring(L, "z");
    lua_pushinteger(L, player->movement.position.x);
    lua_settable(L, -3);
    lua_pushstring(L, "y");
    lua_pushinteger(L, player->movement.position.y);
    lua_settable(L, -3);
    lua_pushstring(L, "x");
    lua_pushinteger(L, player->movement.position.z);
    lua_settable(L, -3);

    return 1;
}

static int get_blocks(lua_State* L) {
    // Check the number of arguments
    int numArgs = lua_gettop(L);
    if (numArgs < 1) {
        luaL_error(L, "Insufficient arguments. Usage: player:get_blocks() or  player.get_blocks(player)");
        return 0;
    }

    // Retrieve the player table from the first argument
    luaL_checktype(L, 1, LUA_TTABLE);
    // Assuming the player table has an "address" field, retrieve the player's C struct's address
    lua_getfield(L, 1, "address");
    // Check if the value is a userdata
    player_t** playerUserData = (player_t**) lua_touserdata(L, -1);
    player_t* player = *playerUserData;

    /* Return the position of the player. */
    lua_pushinteger(L, player->blocks);

    return 1;
}


// C function for the Lua set_color function
static int set_color_function(lua_State* L)
{

    // Check the number of arguments
    int numArgs = lua_gettop(L);
    if (numArgs < 2) {
        luaL_error(L, "Insufficient arguments. Usage: player:set_color(color) or  player.set_color(player,color)");
        return 0;
    }

    // Retrieve the player table from the first argument
    luaL_checktype(L, 1, LUA_TTABLE);
    // Assuming the player table has an "address" field, retrieve the player's C struct's address
    lua_getfield(L, 1, "address");
    // Check if the value is a userdata
    player_t** playerUserData = (player_t**) lua_touserdata(L, -1);
    player_t*  player;
    player = *playerUserData;

    uint32_t color = luaL_checkinteger(L, 2);

    // Get the player * from the player ID.
    server_t* server = get_server();

    color_t col;
    col.raw            = color;
    player->tool_color = col;

    // Send to every players excepted the one we set, then send to him/
    send_set_color(server, player, col);
    send_set_color_to_player(server, player, player, col);
    return 0;
}

void push_player_api(lua_State* L, player_t* player)
{
    // Push the player table first
    lua_newtable(L);

    // Let's push a few consts:
    lua_pushstring(L, "name");
    lua_pushstring(L, player->name);
    lua_settable(L, -3);

    lua_pushstring(L, "id");
    lua_pushinteger(L, player->id + 1);
    lua_settable(L, -3);

    // Let's push a userdata named address (kinda a C pointer) pointing to the player's struct.
    lua_pushstring(L, "address");
    player_t** playerUserData = (player_t**) lua_newuserdata(L, sizeof(player_t*));
    *playerUserData           = player;
    lua_settable(L, -3);

    // Here are pushed all player's methods.
    // Used as player:myfunc()

    // Sould be renamed "place_block" ?
    lua_pushstring(L, "create_block");
    lua_pushcfunction(L, create_block_function);
    lua_settable(L, -3);

    lua_pushstring(L, "set_color");
    lua_pushcfunction(L, set_color_function);
    lua_settable(L, -3);

    lua_pushstring(L, "restock");
    lua_pushcfunction(L, restock);
    lua_settable(L, -3);

    lua_pushstring(L, "send_notice");
    lua_pushcfunction(L, send_notice);
    lua_settable(L, -3);
    
    lua_pushstring(L, "get_team");
    lua_pushcfunction(L, get_team);
    lua_settable(L, -3);

    lua_pushstring(L, "get_position");
    lua_pushcfunction(L, get_position);
    lua_settable(L, -3);

    lua_pushstring(L, "get_blocks");
    lua_pushcfunction(L, get_blocks);
    lua_settable(L, -3);
}
