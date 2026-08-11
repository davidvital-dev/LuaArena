#include "LuaBindings.hpp"

extern "C" {
#include <lauxlib.h>
}

#include <iostream>

void LuaBindings::registrar(lua_State* L) {
    lua_register(L, "game_log", LuaBindings::game_log);
}

int LuaBindings::game_log(lua_State* L) {
    int argc = lua_gettop(L);

    if (argc != 1 || lua_type(L, 1) != LUA_TSTRING) {
        return luaL_error(L, "game_log espera exatamente um argumento do tipo string.");
    }

    const char* mensagem = lua_tostring(L, 1);
    std::cout << "[game_log] " << mensagem << std::endl;

    return 0;
}
