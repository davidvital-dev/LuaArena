#include "ActionMenu.hpp"
#include "Character.hpp"
#include "Game.hpp"
#include "LuaEngine.hpp"

#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <sstream>
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

Character mage() {
    return {"Mago", 100.0, 20.0, 5.0, 50.0};
}

Character goblin() {
    return {"Goblin", 60.0, 12.0, 3.0, 0.0};
}

void testDamageAndDefense() {
    Character target = mage();

    require(approximately(target.takeDamage(20.0), 15.0), "defesa reduz dano");
    require(approximately(target.getHealth(), 85.0), "dano altera vida");

    const double healthBeforeInvalidDamage = target.getHealth();
    require(approximately(target.takeDamage(0.0), 0.0), "dano zero rejeitado");
    require(approximately(target.takeDamage(-10.0), 0.0), "dano negativo rejeitado");
    require(
        approximately(
            target.takeDamage(std::numeric_limits<double>::quiet_NaN()),
            0.0
        ),
        "dano NaN rejeitado"
    );
    require(
        approximately(
            target.takeDamage(std::numeric_limits<double>::infinity()),
            0.0
        ),
        "dano infinito rejeitado"
    );
    require(
        approximately(target.getHealth(), healthBeforeInvalidDamage),
        "dano invalido nao altera vida"
    );

    require(approximately(target.takeDamage(500.0), 85.0), "dano limitado a vida");
    require(approximately(target.getHealth(), 0.0), "vida nao fica negativa");
    require(target.isDefeated(), "vida zero derrota personagem");
}

void testHealing() {
    Character target = mage();
    target.takeDamage(45.0);

    require(approximately(target.heal(25.0), 25.0), "cura recupera vida");
    require(approximately(target.getHealth(), 85.0), "cura atualiza vida");
    require(approximately(target.heal(50.0), 15.0), "cura limitada ao maximo");
    require(approximately(target.getHealth(), 100.0), "vida no maximo");
    require(approximately(target.heal(10.0), 0.0), "vida cheia nao recebe cura");
    require(approximately(target.heal(-1.0), 0.0), "cura negativa rejeitada");

    target.takeDamage(500.0);
    require(target.isDefeated(), "alvo derrotado para teste de cura");
    require(approximately(target.heal(20.0), 0.0), "cura nao revive");
    require(approximately(target.getHealth(), 0.0), "derrotado permanece sem vida");
}

void testEnergy() {
    Character character = mage();

    require(character.hasEnoughEnergy(20.0), "energia suficiente");
    require(character.spendEnergy(20.0), "gasto valido aceito");
    require(approximately(character.getEnergy(), 30.0), "energia consumida");
    require(!character.spendEnergy(31.0), "saldo insuficiente rejeitado");
    require(approximately(character.getEnergy(), 30.0), "falha preserva energia");
    require(!character.spendEnergy(-1.0), "custo negativo rejeitado");
    require(
        !character.spendEnergy(std::numeric_limits<double>::infinity()),
        "custo infinito rejeitado"
    );
    require(character.spendEnergy(0.0), "custo zero aceito");

    require(approximately(character.restoreEnergy(10.0), 10.0), "energia recuperada");
    require(
        approximately(character.restoreEnergy(100.0), 10.0),
        "recuperacao limitada ao maximo"
    );
    require(approximately(character.getEnergy(), 50.0), "energia no maximo");
    require(approximately(character.restoreEnergy(-1.0), 0.0), "recuperacao invalida");
}

void testBurningAndPoison() {
    Character target = mage();

    require(!target.applyBurning(0, 5.0), "duracao de queimadura validada");
    require(!target.applyPoison(2, -1.0), "dano de veneno validado");
    require(target.applyBurning(3, 5.0), "queimadura aplicada");
    require(target.applyPoison(2, 3.0), "veneno aplicado");
    require(target.isBurning(), "queimadura ativa");
    require(target.isPoisoned(), "veneno ativo");
    require(target.getStatusEffects().size() == 2, "efeitos coexistem");

    require(approximately(target.processBurning(), 5.0), "queimadura ignora defesa");
    require(approximately(target.processPoison(), 3.0), "veneno causa dano direto");
    require(target.getStatusEffects()[0].remainingTurns == 2, "duracao queimadura reduzida");
    require(target.getStatusEffects()[1].remainingTurns == 1, "duracao veneno reduzida");

    require(target.applyPoison(2, 4.0), "veneno reaplicado");
    require(target.getStatusEffects().size() == 2, "reaplicacao nao duplica efeito");
    require(approximately(target.processPoison(), 4.0), "novo valor de veneno usado");
    require(approximately(target.processPoison(), 4.0), "ultimo turno de veneno");
    require(!target.isPoisoned(), "veneno expirou");

    require(approximately(target.processBurning(), 5.0), "segundo turno queimadura");
    require(approximately(target.processBurning(), 5.0), "ultimo turno queimadura");
    require(!target.isBurning(), "queimadura expirou");
    require(target.getStatusEffects().empty(), "efeitos expirados removidos");
}

void testTurnSequence() {
    Game game{mage(), goblin()};

    require(game.getTurnNumber() == 1, "batalha inicia na rodada um");
    require(game.isPlayerTurn(), "jogador inicia");
    require(game.getCurrentCharacter().getName() == "Mago", "personagem atual jogador");
    require(game.getOpponent().getName() == "Goblin", "oponente inimigo");

    require(game.advanceTurn(), "avanca para inimigo");
    require(game.getTurnNumber() == 1, "rodada mantida no turno inimigo");
    require(game.getTurnOwner() == TurnOwner::Enemy, "turno do inimigo");
    require(game.getCurrentCharacter().getName() == "Goblin", "personagem atual inimigo");

    require(game.advanceTurn(), "avanca para nova rodada");
    require(game.getTurnNumber() == 2, "rodada incrementada depois do inimigo");
    require(game.isPlayerTurn(), "nova rodada volta ao jogador");
}

void testVictoryAndDefeat() {
    Game victory{mage(), goblin()};
    require(victory.getBattleOutcome() == BattleOutcome::InProgress, "batalha em andamento");
    victory.getEnemy().takeDamage(500.0);
    require(victory.isBattleOver(), "vitoria encerra batalha");
    require(victory.hasPlayerWon(), "inimigo derrotado gera vitoria");
    require(!victory.hasPlayerLost(), "vitoria nao e derrota");
    require(!victory.advanceTurn(), "turno nao avanca depois da vitoria");

    Game defeat{mage(), goblin()};
    defeat.getPlayer().takeDamage(500.0);
    require(defeat.isBattleOver(), "derrota encerra batalha");
    require(defeat.hasPlayerLost(), "jogador derrotado gera derrota");
    require(!defeat.hasPlayerWon(), "derrota nao e vitoria");
    require(!defeat.advanceTurn(), "turno nao avanca depois da derrota");
}

void testActionMenu() {
    ActionMenu menu{{
        {"bola_de_fogo", "Bola de Fogo"},
        {"cura", "Cura"},
        {"golpe_venenoso", "Golpe Venenoso"},
    }};

    std::ostringstream displayedMenu;
    menu.display(displayedMenu);
    require(displayedMenu.str().find("1. Ataque basico") != std::string::npos, "ataque no menu");
    require(displayedMenu.str().find("4. Golpe Venenoso") != std::string::npos, "habilidades no menu");

    std::istringstream invalidThenValid{"texto\n0\n2 extra\n4\n"};
    std::ostringstream output;
    const auto ability = menu.readSelection(invalidThenValid, output);
    require(ability.has_value(), "entrada valida produz selecao");
    require(ability->type == PlayerActionType::Ability, "habilidade selecionada");
    require(ability->abilityIdentifier == "golpe_venenoso", "identificador preservado");
    require(output.str().find("Opcao invalida") != std::string::npos, "entrada invalida avisada");

    std::istringstream basicAttackInput{"1\n"};
    std::ostringstream basicAttackOutput;
    const auto basicAttack = menu.readSelection(basicAttackInput, basicAttackOutput);
    require(basicAttack.has_value(), "ataque basico selecionado");
    require(basicAttack->type == PlayerActionType::BasicAttack, "tipo ataque basico");
    require(basicAttack->abilityIdentifier.empty(), "ataque nao possui habilidade");

    std::istringstream endOfInput;
    std::ostringstream endOutput;
    require(!menu.readSelection(endOfInput, endOutput), "EOF cancela menu");

    bool duplicateRejected = false;
    try {
        ActionMenu duplicate{{{"cura", "Cura"}, {"cura", "Outra Cura"}}};
        (void)duplicate;
    } catch (const std::invalid_argument&) {
        duplicateRejected = true;
    }
    require(duplicateRejected, "identificador duplicado rejeitado");

    bool emptyEntryRejected = false;
    try {
        ActionMenu empty{{{"", "Sem identificador"}}};
        (void)empty;
    } catch (const std::invalid_argument&) {
        emptyEntryRejected = true;
    }
    require(emptyEntryRejected, "habilidade vazia rejeitada");
}

void testAbilityApplication() {
    LuaEngine engine;
    require(
        engine.loadScript("scripts/abilities/abilities.lua"),
        "script de habilidades deve carregar: " + engine.getLastError()
    );

    Game fire{mage(), goblin()};
    AbilityResult fireResult;
    require(
        fire.useAbility(engine, "bola_de_fogo", fireResult),
        "Bola de Fogo deve ser aplicada: " + fireResult.message
    );
    require(fireResult.success && fireResult.applied, "resultado confirma aplicação");
    require(approximately(fire.getPlayer().getEnergy(), 30.0), "custo descontado por C++");
    require(approximately(fire.getEnemy().getHealth(), 33.0), "dano respeita defesa");
    require(fire.getEnemy().isBurning(), "queimadura aplicada por C++");
    require(
        approximately(fire.getEnemy().processBurning(), 30.0),
        "queimadura usa candidate.damage como dano por turno"
    );

    Game healing{mage(), goblin()};
    healing.getPlayer().takeDamage(30.0);
    AbilityResult healingResult;
    require(
        healing.useAbility(engine, "cura", healingResult),
        "Cura deve ser aplicada: " + healingResult.message
    );
    require(healingResult.applied, "Cura marcada como aplicada");
    require(approximately(healing.getPlayer().getEnergy(), 35.0), "custo da Cura");
    require(approximately(healing.getPlayer().getHealth(), 100.0), "cura limitada à vida máxima");

    Game poison{mage(), goblin()};
    AbilityResult poisonResult;
    require(
        poison.useAbility(engine, "golpe_venenoso", poisonResult),
        "Golpe Venenoso deve ser aplicado: " + poisonResult.message
    );
    require(approximately(poison.getEnemy().getHealth(), 48.0), "dano do veneno");
    require(poison.getEnemy().isPoisoned(), "veneno aplicado por C++");
    require(
        approximately(poison.getEnemy().processPoison(), 15.0),
        "veneno usa candidate.damage como dano por turno"
    );

    Game insufficientEnergy{
        {"Mago", 100.0, 20.0, 5.0, 19.0},
        goblin(),
    };
    AbilityResult insufficientResult;
    require(
        !insufficientEnergy.useAbility(engine, "bola_de_fogo", insufficientResult),
        "energia insuficiente não aplica habilidade"
    );
    require(!insufficientResult.success && !insufficientResult.applied, "recusa Lua preservada");
    require(approximately(insufficientEnergy.getPlayer().getEnergy(), 19.0), "falha não gasta energia");
    require(approximately(insufficientEnergy.getEnemy().getHealth(), 60.0), "falha não causa dano");

    Game enemyTurn{mage(), goblin()};
    require(enemyTurn.advanceTurn(), "avança para turno inimigo");
    AbilityResult enemyTurnResult;
    require(
        !enemyTurn.useAbility(engine, "cura", enemyTurnResult),
        "habilidade fora do turno do jogador é recusada"
    );
    require(!enemyTurnResult.applied, "turno inválido não aplica ação");
    require(approximately(enemyTurn.getPlayer().getEnergy(), 50.0), "turno inválido não gasta energia");

    Game endedBattle{mage(), goblin()};
    endedBattle.getEnemy().takeDamage(500.0);
    AbilityResult endedBattleResult;
    require(
        !endedBattle.useAbility(engine, "cura", endedBattleResult),
        "habilidade após fim da batalha é recusada"
    );
    require(!endedBattleResult.applied, "batalha encerrada não aplica ação");

    LuaEngine fixture;
    require(
        fixture.loadScript("tests/fixtures/ability_functions.lua"),
        "fixture deve carregar: " + fixture.getLastError()
    );
    Game revalidated{mage(), goblin()};
    AbilityResult revalidatedResult;
    require(
        !revalidated.useAbility(
            fixture,
            "energia_insuficiente_cpp",
            revalidatedResult
        ),
        "C++ deve revalidar a energia descrita por Lua"
    );
    require(revalidatedResult.success && !revalidatedResult.applied, "Lua válida não basta sem energia C++");
    require(approximately(revalidated.getPlayer().getEnergy(), 50.0), "revalidação preserva energia");
    require(approximately(revalidated.getEnemy().getHealth(), 60.0), "revalidação preserva alvo");

    Game malformed{mage(), goblin()};
    AbilityResult malformedResult;
    require(
        !malformed.useAbility(fixture, "numero_invalido", malformedResult),
        "retorno Lua malformado não aplica habilidade"
    );
    require(!malformedResult.applied, "retorno malformado não é aplicado");
    require(approximately(malformed.getPlayer().getEnergy(), 50.0), "erro Lua não gasta energia");
    require(approximately(malformed.getEnemy().getHealth(), 60.0), "erro Lua não altera inimigo");
}

}  // namespace

int main() {
    const std::vector<std::pair<std::string, std::function<void()>>> tests = {
        {"dano e defesa", testDamageAndDefense},
        {"cura", testHealing},
        {"energia", testEnergy},
        {"queimadura e veneno", testBurningAndPoison},
        {"sequencia de turnos", testTurnSequence},
        {"vitoria e derrota", testVictoryAndDefeat},
        {"menu de acoes", testActionMenu},
        {"integração de habilidades", testAbilityApplication},
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

    std::cout << "Todos os testes do motor passaram.\n";
    return 0;
}
