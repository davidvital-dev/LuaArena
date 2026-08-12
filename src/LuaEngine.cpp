#include "LuaEngine.hpp"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <cmath>
#include <filesystem>
#include <limits>
#include <system_error>

namespace {

bool isFiniteNonNegative(double value) {
    return std::isfinite(value) && value >= 0.0;
}

bool isAllowedActionType(const std::string& type) {
    return type == "ataque" || type == "cura" || type == "defesa" ||
           type == "habilidade" || type == "nenhum";
}

bool isAllowedEffect(const std::string& effect) {
    return effect == "queimadura" || effect == "veneno" ||
           effect == "defesa" || effect == "nenhum";
}

void rawGetField(lua_State* state, int tableIndex, const char* field) {
    tableIndex = lua_absindex(state, tableIndex);
    lua_pushstring(state, field);
    lua_rawget(state, tableIndex);
}

void rawGetGlobal(lua_State* state, const char* name) {
    lua_pushglobaltable(state);
    rawGetField(state, -1, name);
    lua_remove(state, -2);
}

void rawSetStringField(
    lua_State* state,
    int tableIndex,
    const char* field,
    const std::string& value
) {
    tableIndex = lua_absindex(state, tableIndex);
    lua_pushstring(state, field);
    lua_pushlstring(state, value.data(), value.size());
    lua_rawset(state, tableIndex);
}

void rawSetNumberField(
    lua_State* state,
    int tableIndex,
    const char* field,
    double value
) {
    tableIndex = lua_absindex(state, tableIndex);
    lua_pushstring(state, field);
    lua_pushnumber(state, value);
    lua_rawset(state, tableIndex);
}

}  // namespace

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
    rawGetGlobal(state_, functionName.c_str());

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

    return true;
}

bool LuaEngine::pushCharacter(const Character& character) {
    lastError_.clear();

    if (state_ == nullptr) {
        lastError_ = "estado Lua não inicializado";
        return false;
    }

    const double health = character.getHealth();
    const double maximumHealth = character.getMaximumHealth();
    const double attack = character.getAttack();
    const double defense = character.getDefense();
    const double energy = character.getEnergy();
    const double maximumEnergy = character.getMaximumEnergy();

    if (!isFiniteNonNegative(health) ||
        !isFiniteNonNegative(maximumHealth) ||
        !isFiniteNonNegative(attack) ||
        !isFiniteNonNegative(defense) ||
        !isFiniteNonNegative(energy) ||
        !isFiniteNonNegative(maximumEnergy)) {
        lastError_ = "personagem possui atributo numérico inválido para envio ao Lua";
        return false;
    }

    if (health > maximumHealth) {
        lastError_ = "vida atual do personagem supera a vida máxima";
        return false;
    }

    if (energy > maximumEnergy) {
        lastError_ = "energia atual do personagem supera a energia máxima";
        return false;
    }

    lua_createtable(state_, 0, 7);
    const int tableIndex = lua_gettop(state_);

    rawSetStringField(state_, tableIndex, "nome", character.getName());
    rawSetNumberField(state_, tableIndex, "vida", health);
    rawSetNumberField(state_, tableIndex, "vida_maxima", maximumHealth);
    rawSetNumberField(state_, tableIndex, "ataque", attack);
    rawSetNumberField(state_, tableIndex, "defesa", defense);
    rawSetNumberField(state_, tableIndex, "energia", energy);
    rawSetNumberField(state_, tableIndex, "energia_maxima", maximumEnergy);

    return true;
}

bool LuaEngine::callActionFunction(const std::string& functionName, ActionResult& result) {
    lastError_.clear();

    if (!callFunction(functionName, 1)) {
        return false;
    }

    ActionResult candidate;
    const bool valid = readActionResult(-1, candidate);
    lua_pop(state_, 1);

    if (!valid) {
        return false;
    }

    result = candidate;
    return true;
}

bool LuaEngine::readActionResult(int tableIndex, ActionResult& result) {
    if (state_ == nullptr) {
        lastError_ = "estado Lua não inicializado";
        return false;
    }

    tableIndex = lua_absindex(state_, tableIndex);
    const int stackTop = lua_gettop(state_);

    if (!lua_istable(state_, tableIndex)) {
        const char* typeName = lua_typename(state_, lua_type(state_, tableIndex));
        lastError_ = "retorno de ação deve ser table, recebido: "
            + std::string(typeName == nullptr ? "desconhecido" : typeName);
        return false;
    }

    ActionResult candidate;

    rawGetField(state_, tableIndex, "tipo");
    if (lua_type(state_, -1) != LUA_TSTRING) {
        lastError_ = "campo obrigatório 'tipo' deve ser string";
        lua_settop(state_, stackTop);
        return false;
    }
    candidate.type = lua_tostring(state_, -1);
    lua_pop(state_, 1);

    if (!isAllowedActionType(candidate.type)) {
        lastError_ = "campo 'tipo' possui ação não permitida: '" + candidate.type + "'";
        lua_settop(state_, stackTop);
        return false;
    }

    rawGetField(state_, tableIndex, "valor");
    if (lua_type(state_, -1) != LUA_TNUMBER) {
        lastError_ = "campo obrigatório 'valor' deve ser number";
        lua_settop(state_, stackTop);
        return false;
    }
    candidate.value = lua_tonumber(state_, -1);
    lua_pop(state_, 1);

    if (!isFiniteNonNegative(candidate.value)) {
        lastError_ = "campo 'valor' deve ser finito e não negativo";
        lua_settop(state_, stackTop);
        return false;
    }

    rawGetField(state_, tableIndex, "mensagem");
    if (lua_type(state_, -1) != LUA_TSTRING) {
        lastError_ = "campo obrigatório 'mensagem' deve ser string";
        lua_settop(state_, stackTop);
        return false;
    }
    candidate.message = lua_tostring(state_, -1);
    lua_pop(state_, 1);

    rawGetField(state_, tableIndex, "efeito");
    if (lua_type(state_, -1) == LUA_TNIL) {
        candidate.effect.clear();
    } else if (lua_type(state_, -1) == LUA_TSTRING) {
        candidate.effect = lua_tostring(state_, -1);
        if (!isAllowedEffect(candidate.effect)) {
            lastError_ = "campo 'efeito' possui efeito não permitido: '"
                + candidate.effect + "'";
            lua_settop(state_, stackTop);
            return false;
        }
    } else {
        lastError_ = "campo 'efeito' deve ser string ou nil";
        lua_settop(state_, stackTop);
        return false;
    }
    lua_pop(state_, 1);

    rawGetField(state_, tableIndex, "duracao");
    if (lua_type(state_, -1) == LUA_TNIL) {
        candidate.duration = 0;
    } else if (lua_isinteger(state_, -1)) {
        const lua_Integer duration = lua_tointeger(state_, -1);
        if (duration < 0 ||
            duration > static_cast<lua_Integer>(std::numeric_limits<int>::max())) {
            lastError_ = "campo 'duracao' deve ser inteiro não negativo válido";
            lua_settop(state_, stackTop);
            return false;
        }
        candidate.duration = static_cast<int>(duration);
    } else {
        lastError_ = "campo 'duracao' deve ser inteiro ou nil";
        lua_settop(state_, stackTop);
        return false;
    }
    lua_pop(state_, 1);

    rawGetField(state_, tableIndex, "custo");
    if (lua_type(state_, -1) == LUA_TNIL) {
        candidate.energyCost = 0.0;
    } else if (lua_type(state_, -1) == LUA_TNUMBER) {
        candidate.energyCost = lua_tonumber(state_, -1);
        if (!isFiniteNonNegative(candidate.energyCost)) {
            lastError_ = "campo 'custo' deve ser finito e não negativo";
            lua_settop(state_, stackTop);
            return false;
        }
    } else {
        lastError_ = "campo 'custo' deve ser number ou nil";
        lua_settop(state_, stackTop);
        return false;
    }
    lua_pop(state_, 1);

    lua_settop(state_, stackTop);
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
