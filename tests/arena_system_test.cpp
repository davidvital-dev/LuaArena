#include "ArenaConfig.hpp"
#include "ArenaEvent.hpp"
#include "ArenaManager.hpp"
#include "DifficultyConfig.hpp"
#include "LuaEngine.hpp"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

bool approximately(double left, double right) {
    return std::abs(left - right) < 0.000001;
}

ArenaCharacter player(double health = 80.0) {
    return {"Herói", health, 100.0, 20.0, 5.0, 50.0, 50.0};
}

ArenaCharacter enemy(double health = 60.0) {
    return {"Goblin", health, 60.0, 12.0, 3.0, 0.0, 0.0};
}

void applyTo(ArenaCharacter& character, const ArenaEvent& event) {
    if (event.type == ArenaEventType::Damage) {
        character.health = std::max(0.0, character.health - event.value);
    } else if (event.type == ArenaEventType::Healing) {
        character.health = std::min(
            character.maximumHealth,
            character.health + event.value
        );
    }
}

void applyEvent(
    const ArenaEvent& event,
    ArenaCharacter& currentPlayer,
    ArenaCharacter& currentEnemy
) {
    if (event.target == ArenaTarget::Player || event.target == ArenaTarget::All) {
        applyTo(currentPlayer, event);
    }
    if (event.target == ArenaTarget::Enemy || event.target == ArenaTarget::All) {
        applyTo(currentEnemy, event);
    }
}

void testDifficultyLoading() {
    DifficultyLoader loader;
    require(
        loader.load("scripts/difficulty/normal.lua"),
        "dificuldade normal: " + loader.lastError()
    );
    require(approximately(loader.config().healthMultiplier, 1.0), "multiplicador vida");
    require(approximately(loader.config().attackMultiplier, 1.0), "multiplicador ataque");
    require(approximately(loader.config().criticalChance, 0.10), "chance crítico");
    require(loader.config().healingEnabled, "cura habilitada");

    require(
        loader.load("tests/fixtures/invalid_difficulty.lua"),
        "configuração inválida deve usar defaults"
    );
    require(loader.warnings().size() == 4, "um aviso por campo inválido");
    require(approximately(loader.config().healthMultiplier, 1.0), "default vida");
    require(approximately(loader.config().attackMultiplier, 1.0), "default ataque");
    require(approximately(loader.config().criticalChance, 0.10), "default crítico");
    require(loader.config().healingEnabled, "default cura");
}

void testRawContractReads() {
    DifficultyLoader difficulty;
    require(
        difficulty.load("tests/fixtures/metamethod_difficulty.lua"),
        "metamethod de dificuldade nao deve ser executado"
    );
    require(difficulty.warnings().size() == 4, "campos raw ausentes usam defaults");
    require(
        approximately(difficulty.config().healthMultiplier, 1.0),
        "default raw vida"
    );
    require(
        approximately(difficulty.config().attackMultiplier, 1.0),
        "default raw ataque"
    );
    require(
        approximately(difficulty.config().criticalChance, 0.10),
        "default raw critico"
    );
    require(difficulty.config().healingEnabled, "default raw cura");

    ArenaManager manager;
    require(
        !manager.load("tests/fixtures/metamethod_arena_config.lua"),
        "metamethod de configuracao de arena nao deve ser executado"
    );
    require(!manager.lastError().empty(), "configuracao proxy gera erro controlado");

    require(manager.load("tests/fixtures/metamethod_events.lua"), manager.lastError());
    const auto currentPlayer = player();
    const auto currentEnemy = enemy();
    manager.onBattleStart(currentPlayer, currentEnemy);
    const auto event = manager.onTurnStart(1, currentPlayer, currentEnemy);
    require(event.has_value(), "campos opcionais raw ausentes usam defaults");
    require(event->effect == ArenaEffect::None, "efeito raw ausente");
    require(event->duration == 0, "duracao raw ausente");
}

void testArenaConfigsAndSwitching() {
    ArenaManager manager;
    require(manager.load("scripts/arenas/neutral.lua"), manager.lastError());
    require(manager.config() != nullptr, "config da Arena Neutra");
    require(manager.config()->name == "Arena Neutra", "nome da Arena Neutra");
    require(manager.config()->modifiers.empty(), "Arena Neutra sem modificadores");

    require(
        !manager.load("tests/fixtures/invalid_arena_config.lua"),
        "troca inválida deve ser rejeitada"
    );
    require(manager.config()->name == "Arena Neutra", "arena anterior preservada");

    require(manager.load("scripts/arenas/volcanic.lua"), manager.lastError());
    require(manager.config()->name == "Arena Vulcânica", "troca sem recompilação");
    require(
        approximately(manager.config()->modifier(ArenaModifier::FireDamage), 1.25),
        "modificador de fogo"
    );

    require(manager.load("scripts/arenas/poison_forest.lua"), manager.lastError());
    require(manager.config()->name == "Floresta Venenosa", "troca para floresta");
    require(
        approximately(manager.config()->modifier(ArenaModifier::PoisonDamage), 1.10),
        "modificador de veneno"
    );

    require(manager.load("scripts/arenas/healing_temple.lua"), manager.lastError());
    require(manager.config()->name == "Templo de Cura", "troca para templo");
    require(
        approximately(manager.config()->modifier(ArenaModifier::Healing), 1.20),
        "modificador de cura"
    );
}

void testOptionalHooksAndLifecycle() {
    ArenaManager manager;
    const auto currentPlayer = player();
    const auto currentEnemy = enemy();
    require(manager.load("scripts/arenas/neutral.lua"), manager.lastError());
    require(!manager.onBattleStart(currentPlayer, currentEnemy), "hook inicial ausente");
    require(manager.lastError().empty(), "hook ausente não é erro");
    require(!manager.onTurnStart(1, currentPlayer, currentEnemy), "hook turno ausente");
    require(manager.lastError().empty(), "hook de turno ausente não é erro");
    require(
        !manager.onBattleEnd(BattleResult::Victory, currentPlayer, currentEnemy),
        "hook final ausente"
    );
    require(!manager.battleIsActive(), "batalha finalizada");
    require(!manager.onTurnStart(2, currentPlayer, currentEnemy), "evento pós-batalha");
    require(!manager.lastError().empty(), "pós-batalha registrado");
}

void testPeriodicArenaEvents() {
    ArenaManager manager;
    auto currentPlayer = player();
    auto currentEnemy = enemy();

    require(manager.load("scripts/arenas/volcanic.lua"), manager.lastError());
    require(!manager.onBattleStart(currentPlayer, currentEnemy), "início vulcânico");
    require(!manager.onTurnStart(1, currentPlayer, currentEnemy), "vulcânica turno 1");
    const auto heatWave = manager.onTurnStart(3, currentPlayer, currentEnemy);
    require(heatWave.has_value(), "onda de calor no turno 3");
    require(heatWave->target == ArenaTarget::All, "onda de calor em todos");
    require(heatWave->type == ArenaEventType::Damage, "onda de calor causa dano");
    require(approximately(heatWave->value, 5.0), "valor da onda de calor");
    applyEvent(*heatWave, currentPlayer, currentEnemy);
    require(approximately(currentPlayer.health, 75.0), "dano no jogador");
    require(approximately(currentEnemy.health, 55.0), "dano no inimigo");

    require(manager.load("scripts/arenas/poison_forest.lua"), manager.lastError());
    manager.onBattleStart(currentPlayer, currentEnemy);
    const auto poison = manager.onTurnStart(2, currentPlayer, currentEnemy);
    require(poison.has_value(), "esporos no turno 2");
    require(poison->target == ArenaTarget::Player, "esporos no jogador");
    require(poison->effect == ArenaEffect::Poison, "efeito veneno");
    require(poison->duration == 2, "duração do veneno");

    currentPlayer = player(97.0);
    currentEnemy = enemy(58.0);
    require(manager.load("scripts/arenas/healing_temple.lua"), manager.lastError());
    manager.onBattleStart(currentPlayer, currentEnemy);
    const auto healing = manager.onTurnStart(3, currentPlayer, currentEnemy);
    require(healing.has_value(), "cura no turno 3");
    applyEvent(*healing, currentPlayer, currentEnemy);
    require(approximately(currentPlayer.health, 100.0), "cura respeita vida máxima");
    require(approximately(currentEnemy.health, 60.0), "cura do inimigo respeita máximo");
}

void testInvalidLuaData() {
    ArenaManager manager;
    require(
        !manager.load("tests/fixtures/invalid_arena_config.lua"),
        "configuração de arena inválida deve falhar"
    );
    require(!manager.lastError().empty(), "erro da configuração inválida");

    require(
        manager.load("tests/fixtures/unknown_arena_modifier.lua"),
        manager.lastError()
    );
    require(manager.warnings().size() == 1, "modificador desconhecido gera aviso");

    require(manager.load("tests/fixtures/invalid_events.lua"), manager.lastError());
    const auto currentPlayer = player();
    const auto currentEnemy = enemy();
    manager.onBattleStart(currentPlayer, currentEnemy);
    for (int turn = 1; turn <= 8; ++turn) {
        require(!manager.onTurnStart(turn, currentPlayer, currentEnemy), "evento inválido");
        require(!manager.lastError().empty(), "erro controlado do evento inválido");
    }

    const auto recovered = manager.onTurnStart(9, currentPlayer, currentEnemy);
    require(recovered.has_value(), "stack recuperada depois de erros");
    require(recovered->target == ArenaTarget::Enemy, "alvo após recuperação");

    require(
        !manager.onBattleEnd(BattleResult::Defeat, currentPlayer, currentEnemy),
        "hook final com tipo inválido"
    );
    require(!manager.lastError().empty(), "tipo inválido de hook registrado");
    require(!manager.battleIsActive(), "fim ocorre mesmo quando hook é inválido");
}

void testLuaEngineMissingScript() {
    LuaEngine engine;
    require(engine.isInitialized(), "estado Lua inicializado");

    const bool loaded = engine.loadScript("scripts/enemies/goblin_basic.lua");
    require(!loaded, "script inexistente não deve ser considerado carregado");
    require(!engine.getLastError().empty(), "falha de script inexistente gera mensagem");
    require(
        engine.getLastError().find("não encontrado") != std::string::npos,
        "mensagem indica arquivo não encontrado: " + engine.getLastError()
    );

    require(
        engine.loadScript("scripts/abilities/abilities.lua"),
        "engine permanece utilizável após falha: " + engine.getLastError()
    );
}

void testExistingAbilitiesScript() {
    lua_State* state = luaL_newstate();
    require(state != nullptr, "criação do estado Lua para habilidades");
    luaL_openlibs(state);
    const int status = luaL_dofile(state, "tests/abilities_test.lua");
    if (status != LUA_OK) {
        const char* error = lua_tostring(state, -1);
        const std::string message = error == nullptr ? "erro Lua desconhecido" : error;
        lua_close(state);
        throw std::runtime_error("integração com habilidades: " + message);
    }
    lua_close(state);
}

}  // namespace

int main() {
    const std::vector<std::pair<std::string, std::function<void()>>> tests = {
        {"carregamento de dificuldade", testDifficultyLoading},
        {"leitura raw de contratos Lua", testRawContractReads},
        {"configurações e troca de arena", testArenaConfigsAndSwitching},
        {"hooks opcionais e ciclo de vida", testOptionalHooksAndLifecycle},
        {"eventos periódicos", testPeriodicArenaEvents},
        {"dados Lua inválidos", testInvalidLuaData},
        {"script Lua inexistente", testLuaEngineMissingScript},
        {"integração com habilidades", testExistingAbilitiesScript},
    };

    try {
        for (const auto& [name, test] : tests) {
            test();
            std::cout << "[ok] " << name << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << "[falha] " << error.what() << '\n';
        return 1;
    }

    std::cout << "Todos os testes de arenas e integração passaram.\n";
    return 0;
}
