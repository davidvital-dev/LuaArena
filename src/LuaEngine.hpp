#pragma once

struct lua_State;

class LuaEngine {
public:
    LuaEngine() noexcept;
    ~LuaEngine() noexcept;

    LuaEngine(const LuaEngine&) = delete;
    LuaEngine& operator=(const LuaEngine&) = delete;
    LuaEngine(LuaEngine&&) = delete;
    LuaEngine& operator=(LuaEngine&&) = delete;

    bool isInitialized() const noexcept;
    lua_State* getState() noexcept;
    const lua_State* getState() const noexcept;

private:
    lua_State* state_;
};
