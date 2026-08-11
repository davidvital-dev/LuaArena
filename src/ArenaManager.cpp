#include "ArenaManager.hpp"

#include "LuaBindings.hpp"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <cmath>
#include <utility>

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

void pushCharacter(lua_State* state, const ArenaCharacter& character) {
    lua_createtable(state, 0, 7);

    lua_pushlstring(state, character.name.data(), character.name.size());
    lua_setfield(state, -2, "nome");
    lua_pushnumber(state, character.health);
    lua_setfield(state, -2, "vida");
    lua_pushnumber(state, character.maximumHealth);
    lua_setfield(state, -2, "vida_maxima");
    lua_pushnumber(state, character.attack);
    lua_setfield(state, -2, "ataque");
    lua_pushnumber(state, character.defense);
    lua_setfield(state, -2, "defesa");
    lua_pushnumber(state, character.energy);
    lua_setfield(state, -2, "energia");
    lua_pushnumber(state, character.maximumEnergy);
    lua_setfield(state, -2, "energia_maxima");
}

bool readRequiredString(
    lua_State* state,
    int tableIndex,
    const char* field,
    std::string& output,
    std::string& error
) {
    lua_getfield(state, tableIndex, field);
    if (lua_type(state, -1) != LUA_TSTRING) {
        error = std::string("campo obrigatório '") + field + "' deve ser string";
        lua_pop(state, 1);
        return false;
    }

    std::size_t size = 0;
    const char* value = lua_tolstring(state, -1, &size);
    output.assign(value, size);
    lua_pop(state, 1);

    if (output.empty()) {
        error = std::string("campo obrigatório '") + field + "' não pode ser vazio";
        return false;
    }
    return true;
}

bool readRequiredNumber(
    lua_State* state,
    int tableIndex,
    const char* field,
    double& output,
    std::string& error
) {
    lua_getfield(state, tableIndex, field);
    if (lua_type(state, -1) != LUA_TNUMBER) {
        error = std::string("campo obrigatório '") + field + "' deve ser number";
        lua_pop(state, 1);
        return false;
    }

    output = lua_tonumber(state, -1);
    lua_pop(state, 1);
    if (!std::isfinite(output) || output < 0.0) {
        error = std::string("campo obrigatório '") + field
            + "' deve ser finito e não negativo";
        return false;
    }
    return true;
}

std::optional<ArenaTarget> targetFromLua(const std::string& value) {
    if (value == "jogador") {
        return ArenaTarget::Player;
    }
    if (value == "inimigo") {
        return ArenaTarget::Enemy;
    }
    if (value == "todos") {
        return ArenaTarget::All;
    }
    return std::nullopt;
}

std::optional<ArenaEventType> eventTypeFromLua(const std::string& value) {
    if (value == "dano") {
        return ArenaEventType::Damage;
    }
    if (value == "cura") {
        return ArenaEventType::Healing;
    }
    if (value == "defesa") {
        return ArenaEventType::Defense;
    }
    if (value == "nenhum") {
        return ArenaEventType::None;
    }
    return std::nullopt;
}

std::optional<ArenaEffect> effectFromLua(const std::string& value) {
    if (value == "queimadura") {
        return ArenaEffect::Burning;
    }
    if (value == "veneno") {
        return ArenaEffect::Poison;
    }
    if (value == "defesa") {
        return ArenaEffect::Defense;
    }
    if (value == "nenhum") {
        return ArenaEffect::None;
    }
    return std::nullopt;
}

}  // namespace

ArenaManager::ArenaManager() noexcept
    : state_(nullptr), battleActive_(false) {}

ArenaManager::~ArenaManager() noexcept {
    if (state_ != nullptr) {
        lua_close(state_);
    }
}

bool ArenaManager::load(const std::string& scriptPath) {
    lastError_.clear();
    warnings_.clear();

    lua_State* candidate = luaL_newstate();
    if (candidate == nullptr) {
        lastError_ = "não foi possível criar o estado Lua da arena";
        return false;
    }

    luaL_openlibs(candidate);
    LuaBindings::registrar(candidate);

    if (luaL_loadfile(candidate, scriptPath.c_str()) != LUA_OK
        || lua_pcall(candidate, 0, 0, 0) != LUA_OK) {
        setLuaError(candidate, "falha ao carregar arena '" + scriptPath + "'");
        lua_close(candidate);
        return false;
    }

    ArenaConfig candidateConfig;
    if (!readConfig(candidate, candidateConfig)) {
        lua_close(candidate);
        return false;
    }

    if (state_ != nullptr) {
        lua_close(state_);
    }
    state_ = candidate;
    config_ = std::move(candidateConfig);
    battleActive_ = false;
    return true;
}

bool ArenaManager::isLoaded() const noexcept {
    return state_ != nullptr && config_.has_value();
}

bool ArenaManager::battleIsActive() const noexcept {
    return battleActive_;
}

const ArenaConfig* ArenaManager::config() const noexcept {
    return config_ ? &*config_ : nullptr;
}

const std::string& ArenaManager::lastError() const noexcept {
    return lastError_;
}

const std::vector<std::string>& ArenaManager::warnings() const noexcept {
    return warnings_;
}

std::optional<ArenaEvent> ArenaManager::onBattleStart(
    const ArenaCharacter& player,
    const ArenaCharacter& enemy
) {
    lastError_.clear();
    if (!isLoaded()) {
        lastError_ = "nenhuma arena foi carregada";
        return std::nullopt;
    }

    battleActive_ = true;
    return callHook(
        "ao_iniciar_batalha",
        HookArgument::BattleStart,
        0,
        BattleResult::Victory,
        player,
        enemy
    );
}

std::optional<ArenaEvent> ArenaManager::onTurnStart(
    int turn,
    const ArenaCharacter& player,
    const ArenaCharacter& enemy
) {
    lastError_.clear();
    if (!battleActive_) {
        lastError_ = "evento de turno ignorado: batalha não está ativa";
        return std::nullopt;
    }
    if (turn < 1) {
        lastError_ = "turno deve ser um inteiro positivo";
        return std::nullopt;
    }

    return callHook(
        "ao_iniciar_turno",
        HookArgument::TurnStart,
        turn,
        BattleResult::Victory,
        player,
        enemy
    );
}

std::optional<ArenaEvent> ArenaManager::onBattleEnd(
    BattleResult result,
    const ArenaCharacter& player,
    const ArenaCharacter& enemy
) {
    lastError_.clear();
    if (!battleActive_) {
        lastError_ = "evento de fim ignorado: batalha não está ativa";
        return std::nullopt;
    }

    auto event = callHook(
        "ao_finalizar_batalha",
        HookArgument::BattleEnd,
        0,
        result,
        player,
        enemy
    );
    battleActive_ = false;
    return event;
}

bool ArenaManager::readConfig(lua_State* state, ArenaConfig& config) {
    StackGuard guard(state);
    lua_getglobal(state, "arena");
    if (lua_type(state, -1) != LUA_TTABLE) {
        lastError_ = "script deve declarar a tabela global 'arena'";
        return false;
    }

    const int tableIndex = lua_gettop(state);
    if (!readRequiredString(state, tableIndex, "nome", config.name, lastError_)
        || !readRequiredString(
            state,
            tableIndex,
            "descricao",
            config.description,
            lastError_
        )) {
        return false;
    }

    lua_getfield(state, tableIndex, "modificadores");
    if (lua_type(state, -1) != LUA_TTABLE) {
        lastError_ = "campo obrigatório 'modificadores' deve ser table";
        return false;
    }

    lua_pushnil(state);
    while (lua_next(state, -2) != 0) {
        if (lua_type(state, -2) != LUA_TSTRING) {
            lastError_ = "nomes de modificadores devem ser strings";
            return false;
        }

        std::size_t nameSize = 0;
        const char* rawName = lua_tolstring(state, -2, &nameSize);
        const std::string name(rawName, nameSize);
        const auto modifier = arenaModifierFromLuaName(name);
        if (!modifier) {
            warnings_.push_back("modificador desconhecido ignorado: " + name);
            lua_pop(state, 1);
            continue;
        }

        if (lua_type(state, -1) != LUA_TNUMBER) {
            lastError_ = "modificador '" + name + "' deve ser number";
            return false;
        }
        const double value = lua_tonumber(state, -1);
        if (!std::isfinite(value) || value < 0.0) {
            lastError_ = "modificador '" + name + "' deve ser finito e não negativo";
            return false;
        }

        config.modifiers[*modifier] = value;
        lua_pop(state, 1);
    }
    return true;
}

std::optional<ArenaEvent> ArenaManager::callHook(
    const char* hookName,
    HookArgument argument,
    int turn,
    BattleResult result,
    const ArenaCharacter& player,
    const ArenaCharacter& enemy
) {
    StackGuard guard(state_);
    lua_getglobal(state_, hookName);
    if (lua_type(state_, -1) == LUA_TNIL) {
        return std::nullopt;
    }
    if (lua_type(state_, -1) != LUA_TFUNCTION) {
        lastError_ = std::string("hook '") + hookName + "' deve ser function";
        return std::nullopt;
    }

    int argumentCount = 2;
    if (argument == HookArgument::TurnStart) {
        lua_pushinteger(state_, turn);
        argumentCount = 3;
    } else if (argument == HookArgument::BattleEnd) {
        const char* resultText = result == BattleResult::Victory ? "vitoria" : "derrota";
        lua_pushstring(state_, resultText);
        argumentCount = 3;
    }
    pushCharacter(state_, player);
    pushCharacter(state_, enemy);

    if (lua_pcall(state_, argumentCount, 1, 0) != LUA_OK) {
        setLuaError(state_, std::string("erro no hook '") + hookName + "'");
        return std::nullopt;
    }

    if (lua_type(state_, -1) == LUA_TNIL) {
        return std::nullopt;
    }
    if (lua_type(state_, -1) != LUA_TTABLE) {
        lastError_ = std::string("hook '") + hookName + "' deve retornar table ou nil";
        return std::nullopt;
    }

    ArenaEvent event;
    if (!readEvent(state_, event)) {
        lastError_ = std::string("retorno inválido de '") + hookName + "': " + lastError_;
        return std::nullopt;
    }
    return event;
}

bool ArenaManager::readEvent(lua_State* state, ArenaEvent& event) {
    const int tableIndex = lua_gettop(state);
    std::string target;
    std::string type;
    if (!readRequiredString(state, tableIndex, "alvo", target, lastError_)
        || !readRequiredString(state, tableIndex, "tipo", type, lastError_)
        || !readRequiredNumber(state, tableIndex, "valor", event.value, lastError_)
        || !readRequiredString(
            state,
            tableIndex,
            "mensagem",
            event.message,
            lastError_
        )) {
        return false;
    }

    const auto parsedTarget = targetFromLua(target);
    if (!parsedTarget) {
        lastError_ = "alvo desconhecido: " + target;
        return false;
    }
    event.target = *parsedTarget;

    const auto parsedType = eventTypeFromLua(type);
    if (!parsedType) {
        lastError_ = "tipo de evento desconhecido: " + type;
        return false;
    }
    event.type = *parsedType;

    lua_getfield(state, tableIndex, "efeito");
    if (lua_type(state, -1) == LUA_TNIL) {
        event.effect = ArenaEffect::None;
    } else if (lua_type(state, -1) == LUA_TSTRING) {
        std::size_t size = 0;
        const char* rawEffect = lua_tolstring(state, -1, &size);
        const std::string effect(rawEffect, size);
        const auto parsedEffect = effectFromLua(effect);
        if (!parsedEffect) {
            lastError_ = "efeito desconhecido: " + effect;
            lua_pop(state, 1);
            return false;
        }
        event.effect = *parsedEffect;
    } else {
        lastError_ = "campo opcional 'efeito' deve ser string ou nil";
        lua_pop(state, 1);
        return false;
    }
    lua_pop(state, 1);

    lua_getfield(state, tableIndex, "duracao");
    if (lua_type(state, -1) == LUA_TNIL) {
        event.duration = 0;
    } else if (!lua_isinteger(state, -1)) {
        lastError_ = "campo opcional 'duracao' deve ser número inteiro";
        lua_pop(state, 1);
        return false;
    } else {
        const lua_Integer duration = lua_tointeger(state, -1);
        if (duration < 0 || duration > 1000000) {
            lastError_ = "campo opcional 'duracao' está fora do intervalo permitido";
            lua_pop(state, 1);
            return false;
        }
        event.duration = static_cast<int>(duration);
    }
    lua_pop(state, 1);

    if (event.effect == ArenaEffect::None && event.duration != 0) {
        lastError_ = "duração requer um efeito válido";
        return false;
    }
    return true;
}

void ArenaManager::setLuaError(lua_State* state, const std::string& context) {
    const char* message = lua_tostring(state, -1);
    lastError_ = context + ": " + (message == nullptr ? "erro Lua desconhecido" : message);
    lua_pop(state, 1);
}
