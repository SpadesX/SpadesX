#include <Server/Gamemodes/GamemodeLua.h>
#include <Util/Notice.h>
#include <Server/Server.h>
#include <lua.h>
#include <lauxlib.h>

// This file will push a module to lua called "notice".
// It is accessed by using require('notice')

int send_notice_to(lua_State *L) {
    server_t* server = get_server();

    // Check the number of arguments
    int numArgs = lua_gettop(L);
    if (numArgs != 2) {
        return luaL_error(L, "notice.send_to() expects exactly 2 arguments: id (integer) and message (string)");
    }

    // Get the ID (integer) argument
    uint8_t id = (uint8_t) luaL_checkinteger(L, 1);

    // Get the message (string) argument
    const char *message = luaL_checkstring(L, 2);

    // Now you have the ID and message, you can implement your logic here
    // For example, you can send the notice to the specified ID with the given message

    player_t* player;
    HASH_FIND(hh, server->players, &id, sizeof(uint8_t), player);
    if (player == NULL) {
        LOG_ERROR("Could not find player with ID %hhu", id);
        return 0;
    }
    send_server_notice(player, 0, message);
    //void broadcast_server_notice(server_t* server, uint8_t console, const char* message, ...);

    return 0; // Return the number of values pushed onto the Lua stack (in this case, none)
}
int send_notice_broadcast(lua_State * L){
    // Check the number of arguments
    int numArgs = lua_gettop(L);
    if (numArgs != 1) {
        return luaL_error(L, "notice.send_broadcast() expects exactly 1 arguments: id (integer) and message (string)");
    }
    // Get the message (string) argument
    const char *message = luaL_checkstring(L, 1);
    printf("Send broadcast: %s\n", message);
    // Now you have the ID and message, you can implement your logic here
    // For example, you can send the notice to the specified ID with the given message

    return 0; // Return the number of values pushed onto the Lua stack (in this case, none)
}
static const luaL_Reg l_notice[] =
{
    { "send_to", send_notice_to},
    { "send_broadcast", send_notice_broadcast},
    { NULL, NULL }
};


void register_notice_module(lua_State * L){
    luaL_newlib(L, l_notice);
    lua_setglobal(L, "notice");
}