#include <Server/Block.h>
#include <Server/Gamemodes/GamemodeLua.h>
#include <Server/Packets/Packets.h>
#include <Server/Server.h>
#include <Util/Notice.h>


// Send a notice to the player.
// First argument is the player table.
// Second is the message.
static int send_notice(lua_State *L) {
    server_t* server = get_server();

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

    const char *message = luaL_checkstring(L, 2);

    
    // Get the the player * from the id.
    player_t* player;
    HASH_FIND(hh, server->players, &id, sizeof(uint8_t), player);
    if (player == NULL) {
        LOG_ERROR("Could not find player with ID %hhu", id);
        return 0;
    }
    send_server_notice(player, 0, message);
    return 0; // Return the number of values pushed onto the Lua stack (in this case, none)
}

// Restock the player.
static int restock(lua_State* L){
    server_t* server = get_server();

    // Check the number of arguments
    int numArgs = lua_gettop(L);
    if (numArgs < 1) {
        luaL_error(L,
                   "Insufficient arguments. Usage: player:create_block() or  player.create_block(player)");
        return 0;
    }

    // Retrieve the player table from the first argument
    luaL_checktype(L, 1, LUA_TTABLE);
    // Assuming the player table has an "id" field, retrieve the player's ID
    lua_getfield(L, 1, "id");

    uint8_t id = luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    // Get the the player * from the id.
    player_t* player;
    HASH_FIND(hh, server->players, &id, sizeof(uint8_t), player);
    if (player == NULL) {
        LOG_ERROR("Could not find player with ID %hhu", id);
        return 0;
    }
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
    // Assuming the player table has an "id" field, retrieve the player's ID
    lua_getfield(L, 1, "address");
// Check if the value is a userdata
    player_t **playerUserData = (player_t **)lua_touserdata(L, -1);
    player_t* player;
    player = *playerUserData;
    lua_pop(L, 1);

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
    server_t * server = get_server();
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

    lua_pushstring(L, "address");
    //lua_pushinteger(L, (int) (void*) player);
    player_t** playerUserData = (player_t**)lua_newuserdata(L, sizeof(player_t*));
    *playerUserData = player;
    lua_settable(L, -3);

    // Add player's team to the table
    lua_pushstring(L, "team");
    lua_newtable(L); // Create the sub-table containing color and id.
    lua_pushstring(L, "color");
    lua_pushinteger(L, server->protocol.color_team[player->team].raw);
    lua_settable(L, -3);
    lua_pushstring(L, "id");
    lua_pushinteger(L,player->team);
    lua_settable(L, -3);
    lua_settable(L, -3);

    // Add player team to the table
    lua_pushstring(L, "health");
    lua_pushinteger(L, player->hp);
    lua_settable(L, -3);

    // Add player team to the table
    lua_pushstring(L, "blocks");
    lua_pushinteger(L, player->blocks);
    lua_settable(L, -3);

    // Add player team to the table
    lua_pushstring(L, "weapon_pellet");
    lua_pushinteger(L, player->weapon_pellets);
    lua_settable(L, -3);

    // Add player team to the table
    lua_pushstring(L, "weapon_clip");
    lua_pushinteger(L, player->weapon_pellets);
    lua_settable(L, -3);

    // Add player position to the table
    lua_pushstring(L, "position");
    lua_newtable(L); // Create the sub-table
    lua_pushstring(L, "z");
    lua_pushinteger(L, player->movement.position.x);
    lua_settable(L, -3);
    lua_pushstring(L, "y");
    lua_pushinteger(L, player->movement.position.y);
    lua_settable(L, -3);
    lua_pushstring(L, "x");
    lua_pushinteger(L, player->movement.position.z);
    lua_settable(L, -3);
    lua_settable(L, -3);

    // Add create_block function to the table
    lua_pushstring(L, "create_block");
    lua_pushcfunction(L, create_block_function);
    lua_settable(L, -3);

    // Add set_color function to the table
    lua_pushstring(L, "set_color");
    lua_pushcfunction(L, set_color_function);
    lua_settable(L, -3);

    // Add send_notice function to the table
    lua_pushstring(L, "restock");
    lua_pushcfunction(L, restock);
    lua_settable(L, -3);

    // Add send_notice function to the table
    lua_pushstring(L, "send_notice");
    lua_pushcfunction(L, send_notice);
    lua_settable(L, -3);

    // The pushed table is readonly. So you cannot change id, position... as it could break some stuff.
    set_table_as_readonly(L);
}
