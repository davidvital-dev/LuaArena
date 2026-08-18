#pragma once

#include "ArenaEvent.hpp"

#include <string>

class ArenaEventValidator {
public:
    static bool validate(const ArenaEvent& event, std::string& error);
};
