#include "Game.hpp"

#include <limits>
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

int Game::getTurnNumber() const noexcept {
    return turnNumber_;
}

TurnOwner Game::getTurnOwner() const noexcept {
    return turnOwner_;
}

bool Game::isPlayerTurn() const noexcept {
    return turnOwner_ == TurnOwner::Player;
}

Character& Game::getCurrentCharacter() noexcept {
    return isPlayerTurn() ? player_ : enemy_;
}

const Character& Game::getCurrentCharacter() const noexcept {
    return isPlayerTurn() ? player_ : enemy_;
}

Character& Game::getOpponent() noexcept {
    return isPlayerTurn() ? enemy_ : player_;
}

const Character& Game::getOpponent() const noexcept {
    return isPlayerTurn() ? enemy_ : player_;
}

BattleOutcome Game::getBattleOutcome() const noexcept {
    if (player_.isDefeated()) {
        return BattleOutcome::Defeat;
    }
    if (enemy_.isDefeated()) {
        return BattleOutcome::Victory;
    }
    return BattleOutcome::InProgress;
}

bool Game::isBattleOver() const noexcept {
    return getBattleOutcome() != BattleOutcome::InProgress;
}

bool Game::hasPlayerWon() const noexcept {
    return getBattleOutcome() == BattleOutcome::Victory;
}

bool Game::hasPlayerLost() const noexcept {
    return getBattleOutcome() == BattleOutcome::Defeat;
}

bool Game::advanceTurn() noexcept {
    if (isBattleOver()) {
        return false;
    }

    if (isPlayerTurn()) {
        turnOwner_ = TurnOwner::Enemy;
        return true;
    }

    if (turnNumber_ == std::numeric_limits<int>::max()) {
        return false;
    }

    ++turnNumber_;
    turnOwner_ = TurnOwner::Player;
    return true;
}
