#include "LuaEngine.hpp"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <filesystem>
#include <system_error>

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

    std::error_code fsError;
    if (!std::filesystem::is_regular_file(scriptPath, fsError) || fsError) {
        lastError_ = "script Lua não encontrado: '" + scriptPath + "'";
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

bool LuaEngine::callFunction(const std::string& functionName, int resultCount) {
    lastError_.clear();

    if (state_ == nullptr) {
        lastError_ = "estado Lua não inicializado";
        return false;
    }

    if (functionName.empty()) {
        lastError_ = "nome da função Lua não pode ser vazio";
        return false;
    }

    if (resultCount < 0) {
        lastError_ = "quantidade de retornos não pode ser negativa";
        return false;
    }

    const int stackTop = lua_gettop(state_);
    lua_getglobal(state_, functionName.c_str());

    if (lua_isnil(state_, -1)) {
        lastError_ = "função Lua não encontrada: '" + functionName + "'";
        lua_settop(state_, stackTop);
        return false;
    }

    if (!lua_isfunction(state_, -1)) {
        lastError_ = "'" + functionName + "' existe, mas não é uma função Lua";
        lua_settop(state_, stackTop);
        return false;
    }

    if (lua_pcall(state_, 0, resultCount, 0) != LUA_OK) {
        const char* message = lua_tostring(state_, -1);
        lastError_ = "erro ao executar função '" + functionName + "': "
            + (message == nullptr ? "erro Lua desconhecido" : message);
        lua_settop(state_, stackTop);
        return false;
    }

    // lua_pcall já deixa exatamente resultCount valores na stack.
    return true;
}

bool LuaEngine::callActionFunction(const std::string& functionName, ActionResult& result) {
    lastError_.clear();

    if (!callFunction(functionName, 1)) {
        return false;
    }

    if (!lua_istable(state_, -1)) {
        const char* typeName = lua_typename(state_, lua_type(state_, -1));
        lastError_ = "retorno de '" + functionName + "' deve ser table, recebido: "
            + (typeName == nullptr ? "desconhecido" : typeName);
        lua_pop(state_, 1);
        return false;
    }

    const int tableIndex = lua_gettop(state_);
    ActionResult candidate;

    lua_getfield(state_, tableIndex, "tipo");
    if (lua_type(state_, -1) != LUA_TSTRING) {
        lastError_ = "campo 'tipo' ausente ou não é string no retorno de '" + functionName + "'";
        lua_pop(state_, 2);
        return false;
    }
    candidate.type = lua_tostring(state_, -1);
    lua_pop(state_, 1);

    lua_getfield(state_, tableIndex, "valor");
    if (lua_type(state_, -1) != LUA_TNUMBER) {
        lastError_ = "campo 'valor' ausente ou não é number no retorno de '" + functionName + "'";
        lua_pop(state_, 2);
        return false;
    }
    candidate.value = lua_tonumber(state_, -1);
    lua_pop(state_, 1);

    lua_getfield(state_, tableIndex, "mensagem");
    const int messageType = lua_type(state_, -1);
    if (messageType == LUA_TNIL) {
        candidate.message.clear();
    } else if (messageType == LUA_TSTRING) {
        candidate.message = lua_tostring(state_, -1);
    } else {
        lastError_ = "campo 'mensagem' presente mas não é string no retorno de '" + functionName + "'";
        lua_pop(state_, 2);
        return false;
    }
    lua_pop(state_, 1);

    lua_getfield(state_, tableIndex, "efeito");
    const int effectType = lua_type(state_, -1);
    if (effectType == LUA_TNIL) {
        candidate.effect.clear();
    } else if (effectType == LUA_TSTRING) {
        candidate.effect = lua_tostring(state_, -1);
    } else {
        lastError_ = "campo 'efeito' presente mas não é string no retorno de '" + functionName + "'";
        lua_pop(state_, 2);
        return false;
    }
    lua_pop(state_, 1);

    lua_getfield(state_, tableIndex, "duracao");
    const int durationType = lua_type(state_, -1);
    if (durationType == LUA_TNIL) {
        candidate.duration = 0;
    } else if (durationType == LUA_TNUMBER && lua_isinteger(state_, -1)) {
        candidate.duration = static_cast<int>(lua_tointeger(state_, -1));
    } else {
        lastError_ = "campo 'duracao' presente mas não é inteiro no retorno de '" + functionName + "'";
        lua_pop(state_, 2);
        return false;
    }
    lua_pop(state_, 1);

    lua_getfield(state_, tableIndex, "custo");
    const int costType = lua_type(state_, -1);
    if (costType == LUA_TNIL) {
        candidate.energyCost = 0.0;
    } else if (costType == LUA_TNUMBER) {
        candidate.energyCost = lua_tonumber(state_, -1);
    } else {
        lastError_ = "campo 'custo' presente mas não é number no retorno de '" + functionName + "'";
        lua_pop(state_, 2);
        return false;
    }
    lua_pop(state_, 1);

    lua_pop(state_, 1);  // remove a table de retorno
    result = candidate;
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
