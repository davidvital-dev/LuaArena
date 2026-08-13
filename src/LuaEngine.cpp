#include "LuaEngine.hpp"

#include "Character.hpp"

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
    if (!pushCharacter(player)) {
        return false;
    }
    if (!pushCharacter(enemy)) {
        return false;
    }

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
