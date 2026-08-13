// Ponto de entrada do LuaArena: monta a batalha, roda o loop até vitória ou
// derrota e usa a LuaEngine para consultar a decisão do inimigo a cada
// turno. Trocar o script de inimigo (argv[1]) muda o comportamento do
// inimigo sem exigir recompilação do C++.
#include "ActionMenu.hpp"
#include "ActionResult.hpp"
#include "AbilityResult.hpp"
#include "Character.hpp"
#include "Game.hpp"
#include "LuaEngine.hpp"

#include <iostream>
#include <optional>
#include <string>

namespace {

constexpr const char* kAbilitiesScript = "scripts/abilities/abilities.lua";

void printBattleState(const Game& game) {
    const Character& player = game.getPlayer();
    const Character& enemy = game.getEnemy();

    std::cout << "\n-- Turno " << game.getTurnNumber() << " --\n"
              << player.getName() << ": " << player.getHealth() << "/"
              << player.getMaximumHealth() << " vida, " << player.getEnergy()
              << "/" << player.getMaximumEnergy() << " energia\n"
              << enemy.getName() << ": " << enemy.getHealth() << "/"
              << enemy.getMaximumHealth() << " vida\n";
}

// Aplica o ActionResult já validado pela LuaEngine no estado C++. A
// LuaEngine só descreve a ação; quem decide o que acontece com vida,
// energia e status é sempre o C++.
void applyEnemyAction(Game& game, const ActionResult& action) {
    Character& player = game.getPlayer();
    Character& enemy = game.getEnemy();

    std::cout << action.message << '\n';

    if (action.type == "ataque") {
        player.takeDamage(action.value);
    } else if (action.type == "cura") {
        enemy.heal(action.value);
    }

    if (!game.isBattleOver()) {
        if (action.effect == "queimadura") {
            player.applyBurning(action.duration, action.value);
        } else if (action.effect == "veneno") {
            player.applyPoison(action.duration, action.value);
        }
    }
}

void runEnemyTurn(Game& game, LuaEngine& enemyEngine) {
    ActionResult decision;

    if (!enemyEngine.callEnemyActionFunction(
            game.getEnemy(),
            game.getPlayer(),
            decision
        )) {
        std::cout << "Inimigo não pôde agir (" << enemyEngine.getLastError()
                  << "). Turno perdido.\n";
        return;
    }

    applyEnemyAction(game, decision);
}

void runPlayerTurn(Game& game, LuaEngine& abilityEngine, const ActionMenu& menu) {
    const std::optional<PlayerActionSelection> selection =
        menu.readSelection(std::cin, std::cout);

    if (!selection) {
        std::cout << "Entrada encerrada.\n";
        return;
    }

    if (selection->type == PlayerActionType::BasicAttack) {
        Character& player = game.getPlayer();
        Character& enemy = game.getEnemy();
        std::cout << player.getName() << " ataca com um golpe básico.\n";
        enemy.takeDamage(player.getAttack());
        return;
    }

    AbilityResult result;
    if (!game.useAbility(abilityEngine, selection->abilityIdentifier, result)) {
        std::cout << "Habilidade recusada: " << result.message << '\n';
        return;
    }

    std::cout << result.message << '\n';
}

void printOutcome(const Game& game) {
    std::cout << "\n=== Fim de batalha ===\n";
    if (game.hasPlayerWon()) {
        std::cout << game.getPlayer().getName() << " venceu!\n";
    } else if (game.hasPlayerLost()) {
        std::cout << game.getEnemy().getName() << " venceu!\n";
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "uso: " << argv[0] << " <script-de-inimigo.lua>\n";
        return 1;
    }

    const std::string enemyScriptPath = argv[1];

    LuaEngine enemyEngine;
    if (!enemyEngine.isInitialized()) {
        std::cerr << "falha ao inicializar o estado Lua do inimigo\n";
        return 1;
    }
    if (!enemyEngine.loadScript(enemyScriptPath)) {
        std::cerr << "falha ao carregar script de inimigo: "
                  << enemyEngine.getLastError() << '\n';
        return 1;
    }

    LuaEngine abilityEngine;
    if (!abilityEngine.isInitialized()) {
        std::cerr << "falha ao inicializar o estado Lua de habilidades\n";
        return 1;
    }
    if (!abilityEngine.loadScript(kAbilitiesScript)) {
        std::cerr << "falha ao carregar habilidades: "
                  << abilityEngine.getLastError() << '\n';
        return 1;
    }

    Game game{
        Character{"Herói", 100.0, 20.0, 5.0, 50.0},
        Character{"Goblin", 60.0, 12.0, 3.0, 0.0},
    };

    const ActionMenu menu{{
        {"bola_de_fogo", "Bola de Fogo"},
        {"cura", "Cura"},
        {"golpe_venenoso", "Golpe Venenoso"},
    }};

    std::cout << "=== Lua Arena ===\n";
    std::cout << "Inimigo controlado por: " << enemyScriptPath << "\n";

    while (!game.isBattleOver()) {
        printBattleState(game);

        if (game.isPlayerTurn()) {
            runPlayerTurn(game, abilityEngine, menu);
        } else {
            runEnemyTurn(game, enemyEngine);
        }

        if (game.isBattleOver()) {
            break;
        }
        game.advanceTurn();
    }

    printOutcome(game);
    return 0;
}
