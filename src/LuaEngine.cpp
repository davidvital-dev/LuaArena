#include "LuaEngine.hpp"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

LuaEngine::LuaEngine() noexcept
    : state_(luaL_newstate()) {
    if (state_ != nullptr) {
        luaL_openlibs(state_);
    }
}

LuaEngine::~LuaEngine() noexcept {
    if (state_ != nullptr) {
        lua_close(state_);
        state_ = nullptr;
    }
}

bool LuaEngine::isInitialized() const noexcept {
    return state_ != nullptr;
}

bool LuaEngine::loadScript(const std::string& scriptPath) {
    lastError_.clear();

    if (state_ == nullptr) {
        lastError_ = "estado Lua não inicializado";
        return false;
    }

    if (scriptPath.empty()) {
        lastError_ = "caminho do script Lua não pode ser vazio";
        return false;
    }

    const int stackTop = lua_gettop(state_);

    if (luaL_loadfile(state_, scriptPath.c_str()) != LUA_OK) {
        const char* message = lua_tostring(state_, -1);
        lastError_ = "falha ao carregar script '" + scriptPath + "': "
            + (message == nullptr ? "erro Lua desconhecido" : message);
        lua_settop(state_, stackTop);
        return false;
    }

    if (lua_pcall(state_, 0, 0, 0) != LUA_OK) {
        const char* message = lua_tostring(state_, -1);
        lastError_ = "erro ao executar script '" + scriptPath + "': "
            + (message == nullptr ? "erro Lua desconhecido" : message);
        lua_settop(state_, stackTop);
        return false;
    }

    lua_settop(state_, stackTop);
    return true;
}

bool LuaEngine::callFunction(
    const std::string& functionName,
    int argumentCount,
    int resultCount
) {
    lastError_.clear();

    if (state_ == nullptr) {
        lastError_ = "estado Lua não inicializado";
        return false;
    }

    if (functionName.empty()) {
        lastError_ = "nome da função Lua não pode ser vazio";
        return false;
    }

    if (argumentCount < 0 || resultCount < 0) {
        lastError_ = "quantidade de argumentos e retornos não pode ser negativa";
        return false;
    }

    const int stackTop = lua_gettop(state_);
    if (stackTop < argumentCount) {
        lastError_ = "stack Lua não possui argumentos suficientes para chamar '"
            + functionName + "'";
        return false;
    }

    const int baseTop = stackTop - argumentCount;
    lua_getglobal(state_, functionName.c_str());

    if (lua_type(state_, -1) != LUA_TFUNCTION) {
        const int valueType = lua_type(state_, -1);
        if (valueType == LUA_TNIL) {
            lastError_ = "função Lua '" + functionName + "' não encontrada";
        } else {
            const char* typeName = lua_typename(state_, valueType);
            lastError_ = "global Lua '" + functionName + "' não é uma função (tipo: "
                + (typeName == nullptr ? "desconhecido" : typeName) + ")";
        }
        lua_settop(state_, baseTop);
        return false;
    }

    // Coloca a função imediatamente antes dos argumentos já empilhados.
    lua_insert(state_, baseTop + 1);

    if (lua_pcall(state_, argumentCount, resultCount, 0) != LUA_OK) {
        const char* message = lua_tostring(state_, -1);
        lastError_ = "erro ao chamar função Lua '" + functionName + "': "
            + (message == nullptr ? "erro Lua desconhecido" : message);
        lua_settop(state_, baseTop);
        return false;
    }

    return true;
}

const std::string& LuaEngine::getLastError() const noexcept {
    return lastError_;
}

lua_State* LuaEngine::getState() noexcept {
    return state_;
}

const lua_State* LuaEngine::getState() const noexcept {
    return state_;
}
