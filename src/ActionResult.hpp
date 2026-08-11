#pragma once

#include <string>

// Formato de retorno de ações Lua (docs/contracts.md, seção 3).
struct ActionResult {
    std::string type;
    double value = 0.0;
    std::string message;
    std::string effect;
    int duration = 0;
    double energyCost = 0.0;
};
