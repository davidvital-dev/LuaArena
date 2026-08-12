#pragma once

#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

enum class PlayerActionType {
    BasicAttack,
    Ability,
};

struct AbilityMenuEntry {
    std::string identifier;
    std::string displayName;
};

struct PlayerActionSelection {
    PlayerActionType type = PlayerActionType::BasicAttack;
    std::string abilityIdentifier;
};

class ActionMenu {
public:
    explicit ActionMenu(std::vector<AbilityMenuEntry> abilities = {});

    const std::vector<AbilityMenuEntry>& getAbilities() const noexcept;
    void display(std::ostream& output) const;

    // Repete a leitura ate receber uma opcao valida. EOF cancela a selecao.
    std::optional<PlayerActionSelection> readSelection(
        std::istream& input,
        std::ostream& output
    ) const;

private:
    std::vector<AbilityMenuEntry> abilities_;
};
