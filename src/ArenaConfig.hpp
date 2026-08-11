#pragma once

#include <map>
#include <optional>
#include <string>
#include <string_view>

// Modificadores reconhecidos pelo motor. Scripts podem omitir qualquer um
// deles; nesse caso o fator neutro (1.0) é usado.
enum class ArenaModifier {
    FireDamage,
    PoisonDamage,
    Healing,
};

std::optional<ArenaModifier> arenaModifierFromLuaName(std::string_view name) noexcept;
std::string_view arenaModifierLuaName(ArenaModifier modifier) noexcept;

struct ArenaConfig {
    std::string name;
    std::string description;
    std::map<ArenaModifier, double> modifiers;

    double modifier(ArenaModifier key) const noexcept;
};
