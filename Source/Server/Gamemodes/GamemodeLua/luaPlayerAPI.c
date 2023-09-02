#include <Server/Gamemodes/GamemodeLua.h>
#include <Server/Server.h>

void push_player_api(lua_State * L, player_t * player){
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

        // Add player id to the table
        lua_pushstring(L, "team");
        lua_pushinteger(L, player->team);
        lua_settable(L, -3);

        lua_pushstring(L, "position");
        lua_newtable(L); // Create the sub-table
        lua_pushstring(L, "x");
        lua_pushinteger(L, player->movement.position.x); // Replace with the actual value
        lua_settable(L, -3);
        lua_pushstring(L, "y");
        lua_pushinteger(L, player->movement.position.y); // Replace with the actual value
        lua_settable(L, -3);
        lua_settable(L, -3);
}