// Ponto de entrada do LuaArena: monta a batalha, roda o loop até vitória ou
// derrota e usa a LuaEngine para consultar a decisão do inimigo a cada
// turno. Trocar o script de inimigo (argv[1]), a arena (--arena) ou a
// dificuldade (--difficulty) muda o comportamento do jogo sem exigir
// recompilação do C++.
#include "ActionMenu.hpp"
#include "ActionResult.hpp"
#include "AbilityResult.hpp"
#include "ArenaConfig.hpp"
#include "ArenaEvent.hpp"
#include "ArenaManager.hpp"
#include "Character.hpp"
#include "DifficultyConfig.hpp"
#include "Game.hpp"
#include "LuaEngine.hpp"

#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {

constexpr const char* kAbilitiesScript = "scripts/abilities/abilities.lua";
constexpr const char* kDefaultArenaScript = "scripts/arenas/neutral.lua";
constexpr const char* kDefaultDifficultyScript = "scripts/difficulty/normal.lua";
constexpr double kPlayerEnergyRecoveryPerTurn = 5.0;

struct CliOptions {
    std::string enemyScriptPath;
    std::string arenaScriptPath = kDefaultArenaScript;
    std::string difficultyScriptPath = kDefaultDifficultyScript;
};

std::optional<CliOptions> parseArgs(int argc, char** argv) {
    if (argc < 2) {
        return std::nullopt;
    }

    CliOptions options;
    options.enemyScriptPath = argv[1];

    for (int i = 2; i < argc; ++i) {
        const std::string flag = argv[i];
        if (flag == "--arena") {
            if (i + 1 >= argc) {
                std::cerr << "erro: --arena exige um caminho de script\n";
                return std::nullopt;
            }
            options.arenaScriptPath = argv[++i];
        } else if (flag == "--difficulty") {
            if (i + 1 >= argc) {
                std::cerr << "erro: --difficulty exige um caminho de script\n";
                return std::nullopt;
            }
            options.difficultyScriptPath = argv[++i];
        } else {
            std::cerr << "erro: opção desconhecida '" << flag << "'\n";
            return std::nullopt;
        }
    }

    return options;
}

void printWarnings(const std::string& label, const std::vector<std::string>& warnings) {
    for (const std::string& warning : warnings) {
        std::cerr << "aviso (" << label << "): " << warning << '\n';
    }
}

ArenaCharacter toArenaCharacter(const Character& character) {
    return {
        character.getName(),
        character.getHealth(),
        character.getMaximumHealth(),
        character.getAttack(),
        character.getDefense(),
        character.getEnergy(),
        character.getMaximumEnergy(),
    };
}

// Processa queimadura/veneno do personagem da vez, no início do turno dele
// (docs/motor-do-jogo.md, "Fluxo de integração recomendado", passo 4).
void processStatusEffects(Game& game) {
    Character& current = game.getCurrentCharacter();

    if (current.isBurning()) {
        const double damage = current.processBurning();
        std::cout << current.getName() << " sofre " << damage
                  << " de dano por queimadura.\n";
    }

    if (!current.isDefeated() && current.isPoisoned()) {
        const double damage = current.processPoison();
        std::cout << current.getName() << " sofre " << damage
                  << " de dano por veneno.\n";
    }
}

// Recupera parte da energia do herói no início de cada turno dele. O próprio
// Character limita a recuperação à energia máxima, então nunca há sobrecarga.
void restorePlayerEnergy(Game& game) {
    Character& player = game.getPlayer();
    const double restored = player.restoreEnergy(kPlayerEnergyRecoveryPerTurn);

    if (restored > 0.0) {
        std::cout << player.getName() << " recupera " << restored
                  << " de energia.\n";
    }
}

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
void applyEnemyAction(Game& game, const ActionResult& action, const ArenaConfig* arenaConfig) {
    Character& player = game.getPlayer();
    Character& enemy = game.getEnemy();

    if (!enemy.spendEnergy(action.energyCost)) {
        std::cout << "Ação inimiga recusada: energia insuficiente.\n";
        return;
    }

    std::cout << action.message << '\n';

    const double scaledValue = scaleDamageByEffect(arenaConfig, action.effect, action.value);

    if (action.type == "ataque") {
        player.takeDamage(scaledValue);
    } else if (action.type == "cura") {
        enemy.heal(scaleHealing(arenaConfig, action.value));
    }

    if (!game.isBattleOver()) {
        if (action.effect == "queimadura") {
            player.applyBurning(action.duration, scaledValue);
        } else if (action.effect == "veneno") {
            player.applyPoison(action.duration, scaledValue);
        }
    }
}

void runEnemyTurn(
    Game& game,
    LuaEngine& enemyEngine,
    const ArenaConfig* arenaConfig,
    const DifficultyConfig& difficulty
) {
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

    if (decision.type == "cura" && !difficulty.healingEnabled) {
        std::cout << game.getEnemy().getName()
                  << " tentou se curar, mas a dificuldade atual não permite. Turno perdido.\n";
        return;
    }

    applyEnemyAction(game, decision, arenaConfig);
}

bool runPlayerTurn(
    Game& game,
    LuaEngine& abilityEngine,
    const ActionMenu& menu,
    const ArenaConfig* arenaConfig
) {
    const std::optional<PlayerActionSelection> selection =
        menu.readSelection(std::cin, std::cout);

    if (!selection) {
        std::cout << "Entrada encerrada.\n";
        return false;
    }

    if (selection->type == PlayerActionType::BasicAttack) {
        Character& player = game.getPlayer();
        Character& enemy = game.getEnemy();
        std::cout << player.getName() << " ataca com um golpe básico.\n";
        enemy.takeDamage(player.getAttack());
        return true;
    }

    AbilityResult result;
    if (!game.useAbility(abilityEngine, selection->abilityIdentifier, result, arenaConfig)) {
        std::cout << "Habilidade recusada: " << result.message << '\n';
        return true;
    }

    std::cout << result.message << '\n';
    return true;
}

// Aplica um evento de arena (ambiental, fora do controle do jogador ou do
// inimigo) nos personagens-alvo, seguindo os mesmos invariantes do motor:
// dano nunca deixa a vida negativa, cura nunca ultrapassa o máximo, e nada
// é aplicado depois do fim da batalha.
void applyArenaEvent(Game& game, const ArenaEvent& event, const ArenaConfig* arenaConfig) {
    std::cout << event.message << '\n';

    const std::string effect = event.effect == ArenaEffect::Burning ? "queimadura"
        : event.effect == ArenaEffect::Poison ? "veneno"
        : "";
    const double scaledValue = scaleDamageByEffect(arenaConfig, effect, event.value);

    auto applyToTarget = [&](Character& target) {
        if (target.isDefeated()) {
            return;
        }

        if (event.type == ArenaEventType::Damage) {
            target.takeDamage(scaledValue);
        } else if (event.type == ArenaEventType::Healing) {
            target.heal(scaleHealing(arenaConfig, event.value));
        }

        if (!target.isDefeated() && event.duration > 0) {
            if (event.effect == ArenaEffect::Burning) {
                target.applyBurning(event.duration, scaledValue);
            } else if (event.effect == ArenaEffect::Poison) {
                target.applyPoison(event.duration, scaledValue);
            }
        }
    };

    if (event.target == ArenaTarget::Player || event.target == ArenaTarget::All) {
        applyToTarget(game.getPlayer());
    }
    if (!game.isBattleOver() &&
        (event.target == ArenaTarget::Enemy || event.target == ArenaTarget::All)) {
        applyToTarget(game.getEnemy());
    }
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
    const std::optional<CliOptions> options = parseArgs(argc, argv);
    if (!options) {
        std::cerr << "uso: " << argv[0]
                  << " <script-de-inimigo.lua> [--arena <script.lua>] [--difficulty <script.lua>]\n";
        return 1;
    }

    LuaEngine enemyEngine;
    if (!enemyEngine.isInitialized()) {
        std::cerr << "falha ao inicializar o estado Lua do inimigo\n";
        return 1;
    }
    if (!enemyEngine.loadScript(options->enemyScriptPath)) {
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

    ArenaManager arenaManager;
    if (!arenaManager.load(options->arenaScriptPath)) {
        std::cerr << "falha ao carregar arena: " << arenaManager.lastError() << '\n';
        return 1;
    }
    printWarnings("arena", arenaManager.warnings());

    DifficultyLoader difficultyLoader;
    if (!difficultyLoader.load(options->difficultyScriptPath)) {
        std::cerr << "falha ao carregar dificuldade: "
                  << difficultyLoader.lastError() << '\n';
        return 1;
    }
    printWarnings("dificuldade", difficultyLoader.warnings());
    const DifficultyConfig& difficulty = difficultyLoader.config();

    // Balanceamento da batalha padrão: o herói ainda pode vencer com uso
    // inteligente das habilidades, mas o Goblin sobrevive tempo suficiente
    // para pressionar o jogador e ativar seu comportamento agressivo. A energia
    // máxima de 30 e a regeneração gradual evitam tanto spam quanto esgotamento.
    Game game{
        Character{"Herói", 100.0, 15.0, 4.0, 30.0},
        Character{
            "Goblin",
            170.0 * difficulty.healthMultiplier,
            16.0 * difficulty.attackMultiplier,
            5.0,
            0.0
        },
    };

    const ActionMenu menu{{
        {"bola_de_fogo", "Bola de Fogo"},
        {"cura", "Cura"},
        {"golpe_venenoso", "Golpe Venenoso"},
    }};

    const ArenaConfig* arenaConfig = arenaManager.config();

    std::cout << "=== Lua Arena ===\n";
    std::cout << "Inimigo controlado por: " << options->enemyScriptPath << "\n";
    std::cout << "Arena: " << (arenaConfig != nullptr ? arenaConfig->name : "desconhecida") << "\n";
    std::cout << "Dificuldade: vida x" << difficulty.healthMultiplier
              << ", ataque x" << difficulty.attackMultiplier << "\n";

    if (const std::optional<ArenaEvent> startEvent = arenaManager.onBattleStart(
            toArenaCharacter(game.getPlayer()),
            toArenaCharacter(game.getEnemy())
        )) {
        applyArenaEvent(game, *startEvent, arenaConfig);
    }

    while (!game.isBattleOver()) {
        if (game.isPlayerTurn()) {
            restorePlayerEnergy(game);

            if (const std::optional<ArenaEvent> turnEvent = arenaManager.onTurnStart(
                    game.getTurnNumber(),
                    toArenaCharacter(game.getPlayer()),
                    toArenaCharacter(game.getEnemy())
                )) {
                applyArenaEvent(game, *turnEvent, arenaConfig);
                if (game.isBattleOver()) {
                    break;
                }
            }
        }

        processStatusEffects(game);
        if (game.isBattleOver()) {
            break;
        }

        printBattleState(game);

        if (game.isPlayerTurn()) {
            if (!runPlayerTurn(game, abilityEngine, menu, arenaConfig)) {
                return 0;
            }
        } else {
            runEnemyTurn(game, enemyEngine, arenaConfig, difficulty);
        }

        if (game.isBattleOver()) {
            break;
        }
        game.advanceTurn();
    }

    arenaManager.onBattleEnd(
        game.hasPlayerWon() ? BattleResult::Victory : BattleResult::Defeat,
        toArenaCharacter(game.getPlayer()),
        toArenaCharacter(game.getEnemy())
    );

    printOutcome(game);
    return 0;
}
