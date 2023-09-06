#ifndef LUA_UTILS
#define LUA_UTILS

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

// LUA utils... Should be moved later.
// Set table on top of the stack as read-only.
// It does NOT pop it at the end.
// The table on the top of the stack at the end is NOT the one that was there at the begining.
// It is replaced by another one. So do not store refs to the first one.
static inline int l_read_only(lua_State* L)
{
    lua_settop(L, 0);
    printf("Error: attempted to edit a readonly table.\n");
    return 0;
}
// Take the table on top of the stack and set it readonly.
static inline void set_table_as_readonly(lua_State* L)
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

// Funcs used to push stuff on the stack.
void push_init_api(lua_State * L);
void push_player_api(lua_State * L, player_t * player);
int register_spadesx_module(lua_State * L);
void push_block(lua_State * L, int x, int y, int z, uint32_t color);

#endif