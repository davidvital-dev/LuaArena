#pragma once

#include "Character.hpp"

enum class TurnOwner {
    Player,
    Enemy,
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

    // Alterna jogador/inimigo; uma nova rodada comeca depois do inimigo.
    bool advanceTurn() noexcept;

private:
    Character player_;
    Character enemy_;
    int turnNumber_ = 1;
    TurnOwner turnOwner_ = TurnOwner::Player;
};
