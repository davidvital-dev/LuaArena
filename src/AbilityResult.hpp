#pragma once

#include <string>

// Descrição validada de uma habilidade Lua. `success` reproduz o campo
// `sucesso` do script; `applied` só se torna verdadeiro quando o Game confirma
// e aplica a ação no estado C++.
struct AbilityResult {
    bool success = false;
    bool applied = false;
    std::string message;
    double energyCost = 0.0;
    double damage = 0.0;
    double healing = 0.0;
    std::string effect;
    int duration = 0;
};
