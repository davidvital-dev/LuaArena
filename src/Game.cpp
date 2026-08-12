#include "Game.hpp"

#include "LuaEngine.hpp"

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

bool Game::useAbility(
    LuaEngine& engine,
    const std::string& abilityIdentifier,
    AbilityResult& result
) {
    result = AbilityResult{};

    if (isBattleOver()) {
        result.message = "Habilidade ignorada: a batalha já terminou.";
        return false;
    }
    if (!isPlayerTurn()) {
        result.message = "Habilidade ignorada: não é o turno do jogador.";
        return false;
    }
    if (abilityIdentifier.empty()) {
        result.message = "Habilidade inválida.";
        return false;
    }

    AbilityResult candidate;
    if (!engine.callAbilityFunction(
            abilityIdentifier,
            player_,
            enemy_,
            candidate
        )) {
        result.message = engine.getLastError();
        return false;
    }

    if (!candidate.success) {
        result = candidate;
        return false;
    }

    if (!player_.hasEnoughEnergy(candidate.energyCost)) {
        result = candidate;
        result.message = "Energia insuficiente.";
        return false;
    }
    if (!player_.spendEnergy(candidate.energyCost)) {
        result = candidate;
        result.message = "Não foi possível descontar a energia da habilidade.";
        return false;
    }

    enemy_.takeDamage(candidate.damage);
    if (!isBattleOver()) {
        player_.heal(candidate.healing);

        if (candidate.effect == "queimadura") {
            enemy_.applyBurning(candidate.duration, candidate.damage);
        } else if (candidate.effect == "veneno") {
            enemy_.applyPoison(candidate.duration, candidate.damage);
        }
    }

    candidate.applied = true;
    result = candidate;
    return true;
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
