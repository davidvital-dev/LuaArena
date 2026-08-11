#pragma once

#include <string>

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
    bool loadScript(const std::string& scriptPath);

    // Chama uma função global Lua. Os argumentos, quando existirem, devem
    // estar previamente empilhados no estado Lua. Em caso de sucesso, os
    // retornos solicitados permanecem na stack para serem convertidos pelo C++.
    bool callFunction(
        const std::string& functionName,
        int argumentCount = 0,
        int resultCount = 0
    );

    const std::string& getLastError() const noexcept;
    lua_State* getState() noexcept;
    const lua_State* getState() const noexcept;

private:
    lua_State* state_;
    std::string lastError_;
};
