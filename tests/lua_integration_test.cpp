#include "Character.hpp"
#include "LuaEngine.hpp"

extern "C" {
#include <lua.h>
}

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

void requireStringField(
    lua_State* state,
    int tableIndex,
    const char* field,
    const std::string& expected
) {
    tableIndex = lua_absindex(state, tableIndex);
    lua_getfield(state, tableIndex, field);
    require(lua_type(state, -1) == LUA_TSTRING, std::string(field) + " deve ser string");
    require(lua_tostring(state, -1) == expected, std::string(field) + " possui valor incorreto");
    lua_pop(state, 1);
}

void requireNumberField(
    lua_State* state,
    int tableIndex,
    const char* field,
    double expected
) {
    tableIndex = lua_absindex(state, tableIndex);
    lua_getfield(state, tableIndex, field);
    require(lua_type(state, -1) == LUA_TNUMBER, std::string(field) + " deve ser number");
    require(approximately(lua_tonumber(state, -1), expected), std::string(field) + " possui valor incorreto");
    lua_pop(state, 1);
}

void testCharacterToLuaTable() {
    LuaEngine engine;
    require(engine.isInitialized(), "estado Lua inicializado");

    Character hero{"Herói", 100.0, 20.0, 5.0, 50.0};
    hero.takeDamage(25.0);
    require(hero.spendEnergy(10.0), "energia preparada para teste");

    lua_State* state = engine.getState();
    const int stackTop = lua_gettop(state);

    require(engine.pushCharacter(hero), engine.getLastError());
    require(lua_gettop(state) == stackTop + 1, "conversão empilha exatamente uma tabela");
    require(lua_istable(state, -1), "Character convertido deve ser table");

    requireStringField(state, -1, "nome", "Herói");
    requireNumberField(state, -1, "vida", 80.0);
    requireNumberField(state, -1, "vida_maxima", 100.0);
    requireNumberField(state, -1, "ataque", 20.0);
    requireNumberField(state, -1, "defesa", 5.0);
    requireNumberField(state, -1, "energia", 40.0);
    requireNumberField(state, -1, "energia_maxima", 50.0);

    lua_pushnumber(state, 1.0);
    lua_setfield(state, -2, "vida");
    require(approximately(hero.getHealth(), 80.0), "alterar tabela Lua não altera Character");

    lua_pop(state, 1);
    require(lua_gettop(state) == stackTop, "stack restaurada após consumir tabela");
}

void testInvalidCharacterIsRejected() {
    LuaEngine engine;
    require(engine.isInitialized(), "estado Lua inicializado");

    Character invalid{"Inválido", -1.0, 10.0, 0.0, 0.0};
    lua_State* state = engine.getState();
    const int stackTop = lua_gettop(state);

    require(!engine.pushCharacter(invalid), "personagem com valor negativo deve ser rejeitado");
    require(!engine.getLastError().empty(), "personagem inválido gera erro controlado");
    require(lua_gettop(state) == stackTop, "falha de conversão não altera stack");
}

void testActionResultContract() {
    LuaEngine engine;
    require(engine.isInitialized(), "estado Lua inicializado");
    require(
        engine.loadScript("tests/fixtures/lua_integration_contract.lua"),
        engine.getLastError()
    );

    ActionResult result;
    require(engine.callActionFunction("acao_valida_contrato", result), engine.getLastError());
    require(result.type == "ataque", "tipo convertido");
    require(approximately(result.value, 12.0), "valor convertido");
    require(result.message == "Ataque validado pelo contrato.", "mensagem convertida");
    require(result.effect == "veneno", "efeito convertido");
    require(result.duration == 2, "duração convertida");
    require(approximately(result.energyCost, 0.0), "custo convertido");
}

void testInvalidActionResults() {
    LuaEngine engine;
    require(engine.isInitialized(), "estado Lua inicializado");
    require(
        engine.loadScript("tests/fixtures/lua_integration_contract.lua"),
        engine.getLastError()
    );

    const std::vector<std::string> invalidFunctions = {
        "acao_sem_mensagem",
        "acao_tipo_invalido",
        "acao_valor_negativo",
        "acao_valor_infinito",
        "acao_efeito_invalido",
        "acao_duracao_negativa",
        "acao_custo_negativo",
        "acao_metatable_hostil",
    };

    for (const std::string& functionName : invalidFunctions) {
        ActionResult untouched;
        untouched.type = "sentinela";
        const int stackTop = lua_gettop(engine.getState());

        require(
            !engine.callActionFunction(functionName, untouched),
            functionName + " deveria ser rejeitada"
        );
        require(!engine.getLastError().empty(), functionName + " deve gerar erro controlado");
        require(untouched.type == "sentinela", "falha não altera ActionResult de saída");
        require(
            lua_gettop(engine.getState()) == stackTop,
            functionName + " deve restaurar a stack"
        );
    }

    ActionResult recovered;
    require(
        engine.callActionFunction("acao_valida_contrato", recovered),
        "engine deve continuar utilizável após retornos inválidos: " + engine.getLastError()
    );
}

void testEnemyActionScriptsDifferForSameState() {
    LuaEngine basicEngine;
    require(basicEngine.isInitialized(), "estado Lua inicializado");
    require(
        basicEngine.loadScript("scripts/enemies/goblin_basic.lua"),
        basicEngine.getLastError()
    );

    LuaEngine aggressiveEngine;
    require(aggressiveEngine.isInitialized(), "estado Lua inicializado");
    require(
        aggressiveEngine.loadScript("scripts/enemies/goblin_aggressive.lua"),
        aggressiveEngine.getLastError()
    );

    const Character enemy{"Goblin", 60.0, 12.0, 3.0, 0.0};
    const Character healthyPlayer{"Herói", 100.0, 20.0, 5.0, 50.0};

    ActionResult basicResult;
    require(
        basicEngine.callEnemyActionFunction(enemy, healthyPlayer, basicResult),
        basicEngine.getLastError()
    );
    require(basicResult.type == "ataque", "goblin_basic: tipo permitido pelo contrato");
    require(approximately(basicResult.value, 12.0), "goblin_basic: valor igual ao ataque-base");

    ActionResult aggressiveResult;
    require(
        aggressiveEngine.callEnemyActionFunction(enemy, healthyPlayer, aggressiveResult),
        aggressiveEngine.getLastError()
    );
    require(aggressiveResult.type == "ataque", "goblin_aggressive: tipo permitido pelo contrato");
    require(approximately(aggressiveResult.value, 12.0), "goblin_aggressive: sem escalada com jogador saudável");

    require(
        basicResult.message != aggressiveResult.message,
        "os dois scripts devem descrever estratégias diferentes para o mesmo estado"
    );

    const Character woundedPlayer{"Herói", 100.0, 20.0, 5.0, 50.0};
    Character wounded = woundedPlayer;
    wounded.takeDamage(70.0);

    ActionResult basicWounded;
    require(
        basicEngine.callEnemyActionFunction(enemy, wounded, basicWounded),
        basicEngine.getLastError()
    );
    require(approximately(basicWounded.value, 12.0), "goblin_basic ignora a vida do jogador");

    ActionResult aggressiveWounded;
    require(
        aggressiveEngine.callEnemyActionFunction(enemy, wounded, aggressiveWounded),
        aggressiveEngine.getLastError()
    );
    require(
        approximately(aggressiveWounded.value, 18.0),
        "goblin_aggressive intensifica o ataque com o jogador abaixo de 50% de vida"
    );
    require(
        !approximately(basicWounded.value, aggressiveWounded.value),
        "mesmo estado de jogador ferido produz decisões diferentes entre os scripts"
    );
}

void testEnemyActionMissingFunction() {
    LuaEngine engine;
    require(engine.isInitialized(), "estado Lua inicializado");
    require(
        engine.loadScript("tests/fixtures/missing_action_function.lua"),
        engine.getLastError()
    );

    const Character enemy{"Goblin", 60.0, 12.0, 3.0, 0.0};
    const Character player{"Herói", 100.0, 20.0, 5.0, 50.0};

    ActionResult untouched;
    untouched.type = "sentinela";
    const int stackTop = lua_gettop(engine.getState());

    require(
        !engine.callEnemyActionFunction(enemy, player, untouched),
        "script sem escolher_acao deve ser rejeitado"
    );
    require(!engine.getLastError().empty(), "função ausente gera erro controlado");
    require(untouched.type == "sentinela", "falha não altera ActionResult de saída");
    require(
        lua_gettop(engine.getState()) == stackTop,
        "chamada com função ausente deve restaurar a stack"
    );
}

void testEnemyActionInvalidReturn() {
    LuaEngine engine;
    require(engine.isInitialized(), "estado Lua inicializado");
    require(
        engine.loadScript("tests/fixtures/invalid_enemy_action.lua"),
        engine.getLastError()
    );

    const Character enemy{"Goblin", 60.0, 12.0, 3.0, 0.0};
    const Character player{"Herói", 100.0, 20.0, 5.0, 50.0};

    ActionResult untouched;
    untouched.type = "sentinela";
    const int stackTop = lua_gettop(engine.getState());

    require(
        !engine.callEnemyActionFunction(enemy, player, untouched),
        "retorno sem o campo obrigatório 'valor' deve ser rejeitado"
    );
    require(!engine.getLastError().empty(), "retorno inválido gera erro controlado");
    require(untouched.type == "sentinela", "falha não altera ActionResult de saída");
    require(
        lua_gettop(engine.getState()) == stackTop,
        "chamada com retorno inválido deve restaurar a stack"
    );

    ActionResult recovered;
    require(
        !engine.callEnemyActionFunction(enemy, player, recovered),
        "engine deve continuar recusando de forma consistente em chamadas seguintes"
    );
}

}  // namespace

int main() {
    const std::vector<std::pair<std::string, std::function<void()>>> tests = {
        {"Character para tabela Lua", testCharacterToLuaTable},
        {"Character inválido", testInvalidCharacterIsRejected},
        {"contrato ActionResult", testActionResultContract},
        {"ActionResult inválido", testInvalidActionResults},
        {"scripts de inimigo divergem para o mesmo estado", testEnemyActionScriptsDifferForSameState},
        {"escolher_acao ausente", testEnemyActionMissingFunction},
        {"escolher_acao com retorno inválido", testEnemyActionInvalidReturn},
    };

    int failures = 0;
    for (const auto& [name, test] : tests) {
        try {
            test();
            std::cout << "[OK] " << name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "[FALHA] " << name << ": " << error.what() << '\n';
        }
    }

    if (failures != 0) {
        std::cerr << failures << " teste(s) falharam.\n";
        return 1;
    }

    std::cout << "Todos os testes de integração C++/Lua passaram.\n";
    return 0;
}
