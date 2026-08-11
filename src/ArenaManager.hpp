#pragma once

#include "ArenaConfig.hpp"
#include "ArenaEvent.hpp"

#include <optional>
#include <string>
#include <vector>

struct lua_State;

class ArenaManager {
public:
    ArenaManager() noexcept;
    ~ArenaManager() noexcept;

    ArenaManager(const ArenaManager&) = delete;
    ArenaManager& operator=(const ArenaManager&) = delete;
    ArenaManager(ArenaManager&&) = delete;
    ArenaManager& operator=(ArenaManager&&) = delete;

    bool load(const std::string& scriptPath);
    bool isLoaded() const noexcept;
    bool battleIsActive() const noexcept;

    const ArenaConfig* config() const noexcept;
    const std::string& lastError() const noexcept;
    const std::vector<std::string>& warnings() const noexcept;

    std::optional<ArenaEvent> onBattleStart(
        const ArenaCharacter& player,
        const ArenaCharacter& enemy
    );
    std::optional<ArenaEvent> onTurnStart(
        int turn,
        const ArenaCharacter& player,
        const ArenaCharacter& enemy
    );
    std::optional<ArenaEvent> onBattleEnd(
        BattleResult result,
        const ArenaCharacter& player,
        const ArenaCharacter& enemy
    );

private:
    enum class HookArgument {
        BattleStart,
        TurnStart,
        BattleEnd,
    };

    bool readConfig(lua_State* state, ArenaConfig& config);
    std::optional<ArenaEvent> callHook(
        const char* hookName,
        HookArgument argument,
        int turn,
        BattleResult result,
        const ArenaCharacter& player,
        const ArenaCharacter& enemy
    );
    bool readEvent(lua_State* state, ArenaEvent& event);
    void setLuaError(lua_State* state, const std::string& context);

    lua_State* state_;
    std::optional<ArenaConfig> config_;
    std::string lastError_;
    std::vector<std::string> warnings_;
    bool battleActive_;
};
