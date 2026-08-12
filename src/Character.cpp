#include "Character.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

Character::Character(
    std::string name,
    double maximumHealth,
    double attack,
    double defense,
    double maximumEnergy
)
    : name_(std::move(name)),
      health_(maximumHealth),
      maximumHealth_(maximumHealth),
      attack_(attack),
      defense_(defense),
      energy_(maximumEnergy),
      maximumEnergy_(maximumEnergy) {}

const std::string& Character::getName() const noexcept {
    return name_;
}

double Character::getHealth() const noexcept {
    return health_;
}

double Character::getMaximumHealth() const noexcept {
    return maximumHealth_;
}

double Character::getAttack() const noexcept {
    return attack_;
}

double Character::getDefense() const noexcept {
    return defense_;
}

double Character::getEnergy() const noexcept {
    return energy_;
}

double Character::getMaximumEnergy() const noexcept {
    return maximumEnergy_;
}

const std::vector<StatusEffect>& Character::getStatusEffects() const noexcept {
    return statusEffects_;
}

bool Character::isDefeated() const noexcept {
    return health_ <= 0.0;
}

double Character::takeDamage(double amount) noexcept {
    if (!std::isfinite(amount) || amount <= 0.0 || isDefeated()) {
        return 0.0;
    }

    const double validDefense =
        std::isfinite(defense_) ? std::max(0.0, defense_) : 0.0;
    const double effectiveDamage = std::max(0.0, amount - validDefense);
    const double previousHealth = health_;

    health_ = std::max(0.0, health_ - effectiveDamage);
    return previousHealth - health_;
}

double Character::heal(double amount) noexcept {
    if (!std::isfinite(amount) || amount <= 0.0 || isDefeated() ||
        !std::isfinite(health_) || !std::isfinite(maximumHealth_) ||
        health_ >= maximumHealth_) {
        return 0.0;
    }

    const double previousHealth = health_;
    health_ = std::min(maximumHealth_, health_ + amount);
    return health_ - previousHealth;
}

bool Character::hasEnoughEnergy(double amount) const noexcept {
    return !isDefeated() && std::isfinite(amount) && amount >= 0.0 &&
           std::isfinite(energy_) && energy_ >= amount;
}

bool Character::spendEnergy(double amount) noexcept {
    if (!hasEnoughEnergy(amount)) {
        return false;
    }

    energy_ = std::max(0.0, energy_ - amount);
    return true;
}

double Character::restoreEnergy(double amount) noexcept {
    if (!std::isfinite(amount) || amount <= 0.0 || isDefeated() ||
        !std::isfinite(energy_) || !std::isfinite(maximumEnergy_) ||
        energy_ >= maximumEnergy_) {
        return 0.0;
    }

    const double previousEnergy = energy_;
    energy_ = std::min(maximumEnergy_, energy_ + amount);
    return energy_ - previousEnergy;
}

bool Character::applyBurning(int duration, double damagePerTurn) noexcept {
    if (duration <= 0 || !std::isfinite(damagePerTurn) ||
        damagePerTurn <= 0.0 || isDefeated()) {
        return false;
    }

    const auto burning = std::find_if(
        statusEffects_.begin(),
        statusEffects_.end(),
        [](const StatusEffect& effect) { return effect.name == "queimadura"; }
    );

    if (burning != statusEffects_.end()) {
        burning->remainingTurns = duration;
        burning->valuePerTurn = damagePerTurn;
    } else {
        statusEffects_.push_back({"queimadura", duration, damagePerTurn});
    }

    return true;
}

bool Character::isBurning() const noexcept {
    return std::any_of(
        statusEffects_.begin(),
        statusEffects_.end(),
        [](const StatusEffect& effect) {
            return effect.name == "queimadura" && effect.remainingTurns > 0;
        }
    );
}

double Character::processBurning() noexcept {
    const auto burning = std::find_if(
        statusEffects_.begin(),
        statusEffects_.end(),
        [](const StatusEffect& effect) { return effect.name == "queimadura"; }
    );

    if (burning == statusEffects_.end() || isDefeated()) {
        return 0.0;
    }

    const double appliedDamage = takeDirectDamage(burning->valuePerTurn);
    --burning->remainingTurns;

    if (burning->remainingTurns <= 0) {
        statusEffects_.erase(burning);
    }

    return appliedDamage;
}

double Character::takeDirectDamage(double amount) noexcept {
    if (!std::isfinite(amount) || amount <= 0.0 || isDefeated()) {
        return 0.0;
    }

    const double previousHealth = health_;
    health_ = std::max(0.0, health_ - amount);
    return previousHealth - health_;
}
