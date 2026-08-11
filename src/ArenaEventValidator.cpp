#include "ArenaEventValidator.hpp"

#include <cmath>

bool ArenaEventValidator::validate(const ArenaEvent& event, std::string& error) {
    if (!std::isfinite(event.value) || event.value < 0.0) {
        error = "valor do evento deve ser finito e não negativo";
        return false;
    }
    if (event.message.empty() || event.message.size() > 4096) {
        error = "mensagem do evento deve conter entre 1 e 4096 bytes";
        return false;
    }
    if (event.duration < 0 || event.duration > 1000000) {
        error = "duração do evento está fora do intervalo permitido";
        return false;
    }
    if (event.effect == ArenaEffect::None && event.duration != 0) {
        error = "duração requer um efeito válido";
        return false;
    }
    if (event.effect != ArenaEffect::None && event.duration == 0) {
        error = "efeito requer duração positiva";
        return false;
    }
    if (event.type == ArenaEventType::None
        && (event.value != 0.0 || event.effect != ArenaEffect::None)) {
        error = "evento do tipo 'nenhum' não pode aplicar valor ou efeito";
        return false;
    }
    return true;
}
