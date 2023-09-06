#include <Server/Gamemodes/GamemodeLua.h>
#include <Server/Server.h>

void push_block(lua_State * L, int x, int y, int z, uint32_t color){
    // Create a new Lua table for the block
    lua_newtable(L);

    // Add x, y, z, and color to the table
    lua_pushstring(L, "x");
    lua_pushinteger(L, x);
    lua_settable(L, -3);

    lua_pushstring(L, "y");
    lua_pushinteger(L, y);
    lua_settable(L, -3);

    lua_pushstring(L, "z");
    lua_pushinteger(L, z);
    lua_settable(L, -3);

    lua_pushstring(L, "color");
    lua_pushinteger(L, color);
    lua_settable(L, -3);
    
    set_table_as_readonly(L);
}