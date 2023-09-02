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

void push_init_api(lua_State * L);
int register_spadesx_module(lua_State * L);

#endif