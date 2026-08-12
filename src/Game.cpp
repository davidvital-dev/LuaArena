#include "Game.hpp"

#include <utility>

Game::Game(Character player, Character enemy)
    : player_(std::move(player)), enemy_(std::move(enemy)) {}

Character& Game::getPlayer() noexcept {
    return player_;
}

const Character& Game::getPlayer() const noexcept {
    return player_;
}

Character& Game::getEnemy() noexcept {
    return enemy_;
}

const Character& Game::getEnemy() const noexcept {
    return enemy_;
}
