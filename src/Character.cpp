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
