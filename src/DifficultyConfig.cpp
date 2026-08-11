#include "DifficultyConfig.hpp"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <cmath>

namespace {

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

void readPositiveNumber(
    lua_State* state,
    int tableIndex,
    const char* field,
    double defaultValue,
    double& output,
    std::vector<std::string>& warnings
) {
    rawGetField(state, tableIndex, field);
    if (lua_type(state, -1) != LUA_TNUMBER) {
        warnings.emplace_back(
            std::string("campo '") + field + "' inválido; usando valor padrão"
        );
        output = defaultValue;
        lua_pop(state, 1);
        return;
    }

    const double value = lua_tonumber(state, -1);
    lua_pop(state, 1);
    if (!std::isfinite(value) || value <= 0.0) {
        warnings.emplace_back(
            std::string("campo '") + field + "' fora do intervalo; usando valor padrão"
        );
        output = defaultValue;
        return;
    }
    output = value;
}

void readProbability(
    lua_State* state,
    int tableIndex,
    const char* field,
    double defaultValue,
    double& output,
    std::vector<std::string>& warnings
) {
    rawGetField(state, tableIndex, field);
    if (lua_type(state, -1) != LUA_TNUMBER) {
        warnings.emplace_back(
            std::string("campo '") + field + "' inválido; usando valor padrão"
        );
        output = defaultValue;
        lua_pop(state, 1);
        return;
    }

    const double value = lua_tonumber(state, -1);
    lua_pop(state, 1);
    if (!std::isfinite(value) || value < 0.0 || value > 1.0) {
        warnings.emplace_back(
            std::string("campo '") + field + "' fora do intervalo; usando valor padrão"
        );
        output = defaultValue;
        return;
    }
    output = value;
}

void readBoolean(
    lua_State* state,
    int tableIndex,
    const char* field,
    bool defaultValue,
    bool& output,
    std::vector<std::string>& warnings
) {
    rawGetField(state, tableIndex, field);
    if (lua_type(state, -1) != LUA_TBOOLEAN) {
        warnings.emplace_back(
            std::string("campo '") + field + "' inválido; usando valor padrão"
        );
        output = defaultValue;
        lua_pop(state, 1);
        return;
    }

    output = lua_toboolean(state, -1) != 0;
    lua_pop(state, 1);
}

}  // namespace

bool DifficultyLoader::load(const std::string& scriptPath) {
    lastError_.clear();
    warnings_.clear();

    lua_State* state = luaL_newstate();
    if (state == nullptr) {
        lastError_ = "não foi possível criar o estado Lua da dificuldade";
        return false;
    }
    luaL_openlibs(state);

    if (luaL_loadfile(state, scriptPath.c_str()) != LUA_OK
        || lua_pcall(state, 0, 0, 0) != LUA_OK) {
        const char* luaError = lua_tostring(state, -1);
        lastError_ = "falha ao carregar dificuldade '" + scriptPath + "': "
            + (luaError == nullptr ? "erro Lua desconhecido" : luaError);
        lua_close(state);
        return false;
    }

    rawGetGlobal(state, "configuracao");
    if (lua_type(state, -1) != LUA_TTABLE) {
        lastError_ = "script deve declarar a tabela global 'configuracao'";
        lua_close(state);
        return false;
    }

    const DifficultyConfig defaults;
    DifficultyConfig candidate;
    const int tableIndex = lua_gettop(state);
    readPositiveNumber(
        state,
        tableIndex,
        "multiplicador_vida",
        defaults.healthMultiplier,
        candidate.healthMultiplier,
        warnings_
    );
    readPositiveNumber(
        state,
        tableIndex,
        "multiplicador_ataque",
        defaults.attackMultiplier,
        candidate.attackMultiplier,
        warnings_
    );
    readProbability(
        state,
        tableIndex,
        "chance_critico",
        defaults.criticalChance,
        candidate.criticalChance,
        warnings_
    );
    readBoolean(
        state,
        tableIndex,
        "cura_habilitada",
        defaults.healingEnabled,
        candidate.healingEnabled,
        warnings_
    );

    config_ = candidate;
    lua_close(state);
    return true;
}

const DifficultyConfig& DifficultyLoader::config() const noexcept {
    return config_;
}

const std::string& DifficultyLoader::lastError() const noexcept {
    return lastError_;
}

const std::vector<std::string>& DifficultyLoader::warnings() const noexcept {
    return warnings_;
}
