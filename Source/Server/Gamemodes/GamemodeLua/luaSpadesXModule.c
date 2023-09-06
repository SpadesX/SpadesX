#include <Server/Gamemodes/GamemodeLua.h>
#include <Util/Notice.h>
#include <Server/Server.h>
#include <lua.h>
#include <lauxlib.h>

// This file will register a module called 'spadesx'
// It is accessed by using require('spadesx')

// Same as previously, but only take a string as argument.
static int send_notice_broadcast(lua_State * L){
    // Check the number of arguments
    int numArgs = lua_gettop(L);
    if (numArgs != 1) {
        return luaL_error(L, "notice.send_broadcast() expects exactly 1 arguments: message (string)");
    }
    // Get the message (string) argument
    const char *message = luaL_checkstring(L, 1);
    server_t* server = get_server();
    broadcast_server_notice(server, 0, message);
    // Now you have the ID and message, you can implement your logic here
    // For example, you can send the notice to the specified ID with the given message

    return 0; // Return the number of values pushed onto the Lua stack (in this case, none)
}

// Function pushed where we need an empty func to be overriden that return 1 by default.
static int return_1_to_be_overriden(lua_State * L){
    // Get the number of arguments on the stack then pop all of them.
    int numArgs = lua_gettop(L);
    lua_pop(L, numArgs);
    // Now return 1:
    lua_pushinteger(L, 1);
    // Return 1 because returning 1 argument.
    return 1;
}

// The default initialisation function.
static int init(lua_State * L){
    // Get the number of arguments on the stack
    int numArgs = lua_gettop(L);
    // Pop all arguments from the stack
    lua_pop(L, numArgs);

    // By default, initialize in ctf mode.
    server_t * server = get_server();
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
    return 0;
}

static const luaL_Reg l_notice[] =
{
    { "send_broadcast", send_notice_broadcast},
    { NULL, NULL }
};
static const luaL_Reg l_block[] =
{
    { "block_destruction", return_1_to_be_overriden},
    { "block_placement", return_1_to_be_overriden},
    { NULL, NULL }
};
static const luaL_Reg l_spadesx[] =
{
    { "init", init},
    { NULL, NULL }
};

static int push_module(lua_State * L){
    const char *name = luaL_checkstring(L, 1);
    LOG_INFO("Registered [%s]\n mdule to lua.", name);

    // The root table:
    luaL_newlib(L, l_spadesx);
    // Now adding sub tables:
    luaL_newlib(L, l_notice);
    lua_setfield(L, -2, "notices");
    luaL_newlib(L, l_block);
    lua_setfield(L, -2, "checks");
    return 1;
}

// Register spadesx as a lua module.
int register_spadesx_module(lua_State * L){
    luaL_requiref(L, "spadesx", push_module, 1);
    return 1;
}