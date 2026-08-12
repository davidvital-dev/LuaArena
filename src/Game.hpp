#pragma once

#include "Character.hpp"

class Game {
public:
    Game(Character player, Character enemy);

    Character& getPlayer() noexcept;
    const Character& getPlayer() const noexcept;

    Character& getEnemy() noexcept;
    const Character& getEnemy() const noexcept;

private:
    Character player_;
    Character enemy_;
};
