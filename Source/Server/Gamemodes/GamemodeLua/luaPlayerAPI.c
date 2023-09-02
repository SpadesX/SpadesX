#include <Server/Block.h>
#include <Server/Gamemodes/GamemodeLua.h>
#include <Server/Packets/Packets.h>
#include <Server/Server.h>

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
    // Assuming the player table has an "id" field, retrieve the player's ID
    lua_getfield(L, 1, "id");
    uint8_t id = luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    // Retrieve x, y, and z arguments
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    int z = luaL_checkinteger(L, 4);

    server_t* server = get_server();
    player_t* player;
    HASH_FIND(hh, server->players, &id, sizeof(uint8_t), player);
    if (player == NULL) {
        LOG_ERROR("Could not find player with ID %hhu", id);
        return 0;
    }
    mapvxl_set_color(&server->s_map.map, x, y, z, player->tool_color.raw);
    moveIntelAndTentUp(server);
    send_block_action(server, player, BLOCKACTION_BUILD, x, y, z);
    send_block_action_to_player(server, player, player, BLOCKACTION_BUILD, x, y, z);

    return 0;
}

// C function for the Lua set_color function
static int set_color_function(lua_State* L)
{
    // Check the number of arguments
    int numArgs = lua_gettop(L);
    if (numArgs < 2) {
        luaL_error(L,
                   "Insufficient arguments. Usage: player:create_block(color) or  player.create_block(player,color)");
        return 0;
    }

    // Retrieve the player table from the first argument
    luaL_checktype(L, 1, LUA_TTABLE);
    // Assuming the player table has an "id" field, retrieve the player's ID
    lua_getfield(L, 1, "id");
    uint8_t id = luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    uint32_t color = luaL_checkinteger(L, 2);

    // Get the player * from the player ID.
    server_t* server = get_server();
    player_t* player;
    HASH_FIND(hh, server->players, &id, sizeof(uint8_t), player);
    if (player == NULL) {
        LOG_ERROR("Could not find player with ID %hhu", id);
        return 0;
    }
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
    // Add player name to the table
    lua_pushstring(L, "name");
    lua_pushstring(L, player->name);
    lua_settable(L, -3);

    // Add player id to the table
    lua_pushstring(L, "id");
    lua_pushinteger(L, player->id);
    lua_settable(L, -3);

    // Add player team to the table
    lua_pushstring(L, "team");
    lua_pushinteger(L, player->team);
    lua_settable(L, -3);

    // Add player position to the table
    lua_pushstring(L, "position");
    lua_newtable(L); // Create the sub-table
    lua_pushstring(L, "z");
    lua_pushinteger(L, player->movement.position.z); // Replace with the actual value
    lua_settable(L, -3);
    lua_pushstring(L, "y");
    lua_pushinteger(L, player->movement.position.y); // Replace with the actual value
    lua_settable(L, -3);
    lua_pushstring(L, "x");
    lua_pushinteger(L, player->movement.position.x); // Replace with the actual value for z
    lua_settable(L, -3);
    lua_settable(L, -3);

    // Add create_block function to the table
    lua_pushstring(L, "create_block");
    lua_pushcfunction(L, create_block_function); // Replace with your implementation
    lua_settable(L, -3);

    // Add set_color function to the table
    lua_pushstring(L, "set_color");
    lua_pushcfunction(L, set_color_function); // Replace with your implementation
    lua_settable(L, -3);
}
