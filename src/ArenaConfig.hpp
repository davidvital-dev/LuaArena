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

// Escala dano de dano/cura pelos modificadores da arena (docs/contracts.md,
// seção 7), usada tanto para ações do jogador quanto do inimigo, para que a
// arena afete os dois lados da batalha de forma simétrica. `config` nulo
// (nenhuma arena carregada) devolve o valor sem alteração.
double scaleDamageByEffect(
    const ArenaConfig* config,
    const std::string& effect,
    double value
) noexcept;
double scaleHealing(const ArenaConfig* config, double value) noexcept;
