#pragma once

#include <string>

#include "AbilityResult.hpp"
#include "ArenaConfig.hpp"
#include "Character.hpp"

class LuaEngine;

enum class TurnOwner {
    Player,
    Enemy,
};

enum class BattleOutcome {
    InProgress,
    Victory,
    Defeat,
};

class Game {
public:
    Game(Character player, Character enemy);

    Character& getPlayer() noexcept;
    const Character& getPlayer() const noexcept;

    Character& getEnemy() noexcept;
    const Character& getEnemy() const noexcept;

    int getTurnNumber() const noexcept;
    TurnOwner getTurnOwner() const noexcept;
    bool isPlayerTurn() const noexcept;

    Character& getCurrentCharacter() noexcept;
    const Character& getCurrentCharacter() const noexcept;
    Character& getOpponent() noexcept;
    const Character& getOpponent() const noexcept;

    BattleOutcome getBattleOutcome() const noexcept;
    bool isBattleOver() const noexcept;
    bool hasPlayerWon() const noexcept;
    bool hasPlayerLost() const noexcept;

    // Executa a habilidade escolhida pelo jogador por meio do LuaEngine. O
    // retorno booleano informa se C++ aplicou a ação; o resultado preserva a
    // descrição Lua e expõe `applied` para quem apresentar a mensagem.
    // `arenaConfig` (opcional) escala dano/cura pelos modificadores da arena
    // atual, para que a arena afete jogador e inimigo de forma simétrica.
    bool useAbility(
        LuaEngine& engine,
        const std::string& abilityIdentifier,
        AbilityResult& result,
        const ArenaConfig* arenaConfig = nullptr
    );

    // Alterna jogador/inimigo; uma nova rodada comeca depois do inimigo.
    bool advanceTurn() noexcept;

private:
    Character player_;
    Character enemy_;
    int turnNumber_ = 1;
    TurnOwner turnOwner_ = TurnOwner::Player;
};
