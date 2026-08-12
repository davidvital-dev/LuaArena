#include "ActionMenu.hpp"

#include <istream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <utility>

ActionMenu::ActionMenu(std::vector<AbilityMenuEntry> abilities)
    : abilities_(std::move(abilities)) {
    std::unordered_set<std::string> identifiers;

    for (const AbilityMenuEntry& ability : abilities_) {
        if (ability.identifier.empty() || ability.displayName.empty()) {
            throw std::invalid_argument(
                "habilidades do menu devem ter identificador e nome"
            );
        }
        if (!identifiers.insert(ability.identifier).second) {
            throw std::invalid_argument(
                "identificador de habilidade duplicado no menu"
            );
        }
    }
}

const std::vector<AbilityMenuEntry>& ActionMenu::getAbilities() const noexcept {
    return abilities_;
}

void ActionMenu::display(std::ostream& output) const {
    output << "Escolha uma acao:\n";
    output << "1. Ataque basico\n";

    for (std::size_t index = 0; index < abilities_.size(); ++index) {
        output << index + 2 << ". " << abilities_[index].displayName << '\n';
    }
}

std::optional<PlayerActionSelection> ActionMenu::readSelection(
    std::istream& input,
    std::ostream& output
) const {
    std::string line;

    while (true) {
        display(output);
        output << "> ";

        if (!std::getline(input, line)) {
            return std::nullopt;
        }

        std::istringstream parser(line);
        std::size_t option = 0;
        if (!(parser >> option)) {
            output << "Opcao invalida. Tente novamente.\n";
            continue;
        }

        parser >> std::ws;
        if (!parser.eof() || option < 1 || option > abilities_.size() + 1) {
            output << "Opcao invalida. Tente novamente.\n";
            continue;
        }

        if (option == 1) {
            return PlayerActionSelection{PlayerActionType::BasicAttack, ""};
        }

        return PlayerActionSelection{
            PlayerActionType::Ability,
            abilities_[option - 2].identifier,
        };
    }
}
