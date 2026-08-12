#include "LuaEngine.hpp"

#include "Character.hpp"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <filesystem>
#include <cmath>
#include <limits>
#include <system_error>

namespace {

class StackGuard {
public:
    explicit StackGuard(lua_State* state) noexcept
        : state_(state), top_(lua_gettop(state)) {}

    ~StackGuard() noexcept {
        lua_settop(state_, top_);
    }

private:
    lua_State* state_;
    int top_;
};

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

void pushCharacter(lua_State* state, const Character& character) {
    lua_createtable(state, 0, 7);

    const std::string& name = character.getName();
    lua_pushlstring(state, name.data(), name.size());
    lua_setfield(state, -2, "nome");
    lua_pushnumber(state, character.getHealth());
    lua_setfield(state, -2, "vida");
    lua_pushnumber(state, character.getMaximumHealth());
    lua_setfield(state, -2, "vida_maxima");
    lua_pushnumber(state, character.getAttack());
    lua_setfield(state, -2, "ataque");
    lua_pushnumber(state, character.getDefense());
    lua_setfield(state, -2, "defesa");
    lua_pushnumber(state, character.getEnergy());
    lua_setfield(state, -2, "energia");
    lua_pushnumber(state, character.getMaximumEnergy());
    lua_setfield(state, -2, "energia_maxima");
}

bool isValidCharacter(const Character& character) {
    const double health = character.getHealth();
    const double maximumHealth = character.getMaximumHealth();
    const double attack = character.getAttack();
    const double defense = character.getDefense();
    const double energy = character.getEnergy();
    const double maximumEnergy = character.getMaximumEnergy();

    return std::isfinite(health) && std::isfinite(maximumHealth) &&
           std::isfinite(attack) && std::isfinite(defense) &&
           std::isfinite(energy) && std::isfinite(maximumEnergy) &&
           health >= 0.0 && maximumHealth >= health && attack >= 0.0 &&
           defense >= 0.0 && energy >= 0.0 && maximumEnergy >= energy;
}

bool readRequiredBoolean(
    lua_State* state,
    int tableIndex,
    const char* field,
    bool& output,
    std::string& error
) {
    rawGetField(state, tableIndex, field);
    if (lua_type(state, -1) != LUA_TBOOLEAN) {
        error = std::string("campo '") + field + "' ausente ou não é boolean";
        lua_pop(state, 1);
        return false;
    }

    output = lua_toboolean(state, -1) != 0;
    lua_pop(state, 1);
    return true;
}

bool readRequiredString(
    lua_State* state,
    int tableIndex,
    const char* field,
    std::string& output,
    std::string& error
) {
    rawGetField(state, tableIndex, field);
    if (lua_type(state, -1) != LUA_TSTRING) {
        error = std::string("campo '") + field + "' ausente ou não é string";
        lua_pop(state, 1);
        return false;
    }

    std::size_t size = 0;
    const char* value = lua_tolstring(state, -1, &size);
    output.assign(value, size);
    lua_pop(state, 1);
    return true;
}

bool readRequiredNonNegativeNumber(
    lua_State* state,
    int tableIndex,
    const char* field,
    double& output,
    std::string& error
) {
    rawGetField(state, tableIndex, field);
    if (lua_type(state, -1) != LUA_TNUMBER) {
        error = std::string("campo '") + field + "' ausente ou não é number";
        lua_pop(state, 1);
        return false;
    }

    output = lua_tonumber(state, -1);
    lua_pop(state, 1);
    if (!std::isfinite(output) || output < 0.0) {
        error = std::string("campo '") + field
            + "' deve ser finito e não negativo";
        return false;
    }
    return true;
}

bool readOptionalNonNegativeNumber(
    lua_State* state,
    int tableIndex,
    const char* field,
    double& output,
    std::string& error
) {
    rawGetField(state, tableIndex, field);
    if (lua_type(state, -1) == LUA_TNIL) {
        output = 0.0;
        lua_pop(state, 1);
        return true;
    }
    if (lua_type(state, -1) != LUA_TNUMBER) {
        error = std::string("campo opcional '") + field + "' deve ser number";
        lua_pop(state, 1);
        return false;
    }

    output = lua_tonumber(state, -1);
    lua_pop(state, 1);
    if (!std::isfinite(output) || output < 0.0) {
        error = std::string("campo opcional '") + field
            + "' deve ser finito e não negativo";
        return false;
    }
    return true;
}

bool readOptionalEffect(
    lua_State* state,
    int tableIndex,
    std::string& output,
    std::string& error
) {
    rawGetField(state, tableIndex, "efeito");
    if (lua_type(state, -1) == LUA_TNIL) {
        output.clear();
        lua_pop(state, 1);
        return true;
    }
    if (lua_type(state, -1) != LUA_TSTRING) {
        error = "campo opcional 'efeito' deve ser string ou nil";
        lua_pop(state, 1);
        return false;
    }

    std::size_t size = 0;
    const char* value = lua_tolstring(state, -1, &size);
    output.assign(value, size);
    lua_pop(state, 1);

    if (output == "nenhum") {
        output.clear();
    }
    if (output.empty() || output == "queimadura" || output == "veneno") {
        return true;
    }

    error = "efeito de habilidade desconhecido: " + output;
    return false;
}

bool readOptionalDuration(
    lua_State* state,
    int tableIndex,
    int& output,
    std::string& error
) {
    rawGetField(state, tableIndex, "duracao");
    if (lua_type(state, -1) == LUA_TNIL) {
        output = 0;
        lua_pop(state, 1);
        return true;
    }
    if (!lua_isinteger(state, -1)) {
        error = "campo opcional 'duracao' deve ser número inteiro";
        lua_pop(state, 1);
        return false;
    }

    const lua_Integer duration = lua_tointeger(state, -1);
    lua_pop(state, 1);
    if (duration < 0 || duration > std::numeric_limits<int>::max()) {
        error = "campo opcional 'duracao' está fora do intervalo permitido";
        return false;
    }

    output = static_cast<int>(duration);
    return true;
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

bool LuaEngine::callAbilityFunction(
    const std::string& abilityName,
    const Character& player,
    const Character& enemy,
    AbilityResult& result
) {
    lastError_.clear();

    if (state_ == nullptr) {
        lastError_ = "estado Lua não inicializado";
        return false;
    }
    if (abilityName.empty()) {
        lastError_ = "nome da habilidade não pode ser vazio";
        return false;
    }
    if (!isValidCharacter(player) || !isValidCharacter(enemy)) {
        lastError_ = "dados dos personagens são inválidos para usar habilidade";
        return false;
    }

    StackGuard guard(state_);
    rawGetGlobal(state_, "usar_habilidade");
    if (lua_type(state_, -1) == LUA_TNIL) {
        lastError_ = "função Lua não encontrada: 'usar_habilidade'";
        return false;
    }
    if (lua_type(state_, -1) != LUA_TFUNCTION) {
        lastError_ = "'usar_habilidade' existe, mas não é uma função Lua";
        return false;
    }

    lua_pushlstring(state_, abilityName.data(), abilityName.size());
    pushCharacter(state_, player);
    pushCharacter(state_, enemy);

    if (lua_pcall(state_, 3, 1, 0) != LUA_OK) {
        const char* message = lua_tostring(state_, -1);
        lastError_ = "erro ao executar função 'usar_habilidade': "
            + std::string(message == nullptr ? "erro Lua desconhecido" : message);
        return false;
    }

    if (lua_type(state_, -1) != LUA_TTABLE) {
        const char* typeName = lua_typename(state_, lua_type(state_, -1));
        lastError_ = "retorno de 'usar_habilidade' deve ser table, recebido: "
            + std::string(typeName == nullptr ? "desconhecido" : typeName);
        return false;
    }

    const int tableIndex = lua_gettop(state_);
    AbilityResult candidate;
    std::string validationError;
    if (!readRequiredBoolean(
            state_,
            tableIndex,
            "sucesso",
            candidate.success,
            validationError
        ) ||
        !readRequiredString(
            state_,
            tableIndex,
            "mensagem",
            candidate.message,
            validationError
        )) {
        lastError_ = "retorno inválido de 'usar_habilidade': " + validationError;
        return false;
    }

    if (!candidate.success) {
        result = candidate;
        return true;
    }

    std::string type;
    if (!readRequiredString(
            state_,
            tableIndex,
            "tipo",
            type,
            validationError
        ) ||
        type != "habilidade") {
        if (validationError.empty()) {
            validationError = "campo 'tipo' deve ser 'habilidade'";
        }
        lastError_ = "retorno inválido de 'usar_habilidade': " + validationError;
        return false;
    }

    if (!readRequiredNonNegativeNumber(
            state_,
            tableIndex,
            "custo",
            candidate.energyCost,
            validationError
        ) ||
        !readOptionalNonNegativeNumber(
            state_,
            tableIndex,
            "dano",
            candidate.damage,
            validationError
        ) ||
        !readOptionalNonNegativeNumber(
            state_,
            tableIndex,
            "cura",
            candidate.healing,
            validationError
        ) ||
        !readOptionalEffect(
            state_,
            tableIndex,
            candidate.effect,
            validationError
        ) ||
        !readOptionalDuration(
            state_,
            tableIndex,
            candidate.duration,
            validationError
        )) {
        lastError_ = "retorno inválido de 'usar_habilidade': " + validationError;
        return false;
    }

    if (candidate.effect.empty() && candidate.duration != 0) {
        lastError_ = "retorno inválido de 'usar_habilidade': duração exige efeito";
        return false;
    }
    if (!candidate.effect.empty() && candidate.duration == 0) {
        lastError_ = "retorno inválido de 'usar_habilidade': efeito exige duração positiva";
        return false;
    }
    if (!candidate.effect.empty() && candidate.damage <= 0.0) {
        lastError_ = "retorno inválido de 'usar_habilidade': efeito exige dano positivo";
        return false;
    }

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
