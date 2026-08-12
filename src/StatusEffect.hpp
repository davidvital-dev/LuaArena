#pragma once

#include <string>

// Efeito temporario armazenado e processado pelo motor do jogo.
struct StatusEffect {
    std::string name;
    int remainingTurns = 0;
    double valuePerTurn = 0.0;
};
