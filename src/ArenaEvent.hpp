#pragma once

#include <string>

enum class ArenaTarget {
    Player,
    Enemy,
    All,
};

enum class ArenaEventType {
    Damage,
    Healing,
    Defense,
    None,
};

enum class ArenaEffect {
    None,
    Burning,
    Poison,
    Defense,
};

struct ArenaCharacter {
    std::string name;
    double health = 0.0;
    double maximumHealth = 0.0;
    double attack = 0.0;
    double defense = 0.0;
    double energy = 0.0;
    double maximumEnergy = 0.0;
};

struct ArenaEvent {
    ArenaTarget target = ArenaTarget::All;
    ArenaEventType type = ArenaEventType::None;
    double value = 0.0;
    std::string message;
    ArenaEffect effect = ArenaEffect::None;
    int duration = 0;
};

enum class BattleResult {
    Victory,
    Defeat,
};
