#pragma once

#include <string>
#include <vector>

#include "StatusEffect.hpp"

class Character {
public:
    Character(
        std::string name,
        double maximumHealth,
        double attack,
        double defense,
        double maximumEnergy
    );

    const std::string& getName() const noexcept;
    double getHealth() const noexcept;
    double getMaximumHealth() const noexcept;
    double getAttack() const noexcept;
    double getDefense() const noexcept;
    double getEnergy() const noexcept;
    double getMaximumEnergy() const noexcept;
    const std::vector<StatusEffect>& getStatusEffects() const noexcept;
    bool isDefeated() const noexcept;

    // Aplica dano apos a reducao pela defesa e retorna a vida removida.
    double takeDamage(double amount) noexcept;

    // Recupera vida sem ultrapassar o limite e retorna a cura efetiva.
    double heal(double amount) noexcept;

    bool hasEnoughEnergy(double amount) const noexcept;
    bool spendEnergy(double amount) noexcept;

    // Recupera energia sem ultrapassar o limite e retorna o valor efetivo.
    double restoreEnergy(double amount) noexcept;

private:
    std::string name_;
    double health_;
    double maximumHealth_;
    double attack_;
    double defense_;
    double energy_;
    double maximumEnergy_;
    std::vector<StatusEffect> statusEffects_;
};
