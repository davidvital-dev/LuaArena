#include "ArenaConfig.hpp"

std::optional<ArenaModifier> arenaModifierFromLuaName(std::string_view name) noexcept {
    if (name == "dano_fogo") {
        return ArenaModifier::FireDamage;
    }
    if (name == "dano_veneno") {
        return ArenaModifier::PoisonDamage;
    }
    if (name == "cura") {
        return ArenaModifier::Healing;
    }
    return std::nullopt;
}

std::string_view arenaModifierLuaName(ArenaModifier modifier) noexcept {
    switch (modifier) {
        case ArenaModifier::FireDamage:
            return "dano_fogo";
        case ArenaModifier::PoisonDamage:
            return "dano_veneno";
        case ArenaModifier::Healing:
            return "cura";
    }
    return "";
}

double ArenaConfig::modifier(ArenaModifier key) const noexcept {
    const auto found = modifiers.find(key);
    return found == modifiers.end() ? 1.0 : found->second;
}
