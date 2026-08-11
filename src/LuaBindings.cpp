#include "LuaBindings.hpp"

extern "C" {
#include <lauxlib.h>
}

#include <iostream>

namespace {

constexpr const char* CURRENT_TURN_REGISTRY_KEY = "lua_arena.turno_atual";

}  // namespace

void LuaBindings::registrar(lua_State* L) {
    lua_register(L, "game_log", LuaBindings::game_log);
    lua_register(L, "obter_turno_atual", LuaBindings::obter_turno_atual);
    definir_turno_atual(L, 0);
}

void LuaBindings::definir_turno_atual(lua_State* L, int turno) {
    lua_pushinteger(L, turno < 0 ? 0 : turno);
    lua_setfield(L, LUA_REGISTRYINDEX, CURRENT_TURN_REGISTRY_KEY);
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

int LuaBindings::obter_turno_atual(lua_State* L) {
    if (lua_gettop(L) != 0) {
        return luaL_error(L, "obter_turno_atual não recebe argumentos.");
    }

    lua_getfield(L, LUA_REGISTRYINDEX, CURRENT_TURN_REGISTRY_KEY);
    if (!lua_isinteger(L, -1)) {
        lua_pop(L, 1);
        lua_pushinteger(L, 0);
    }
    return 1;
}
