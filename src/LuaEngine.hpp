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

    // Chama uma função global sem argumentos nem retorno. Se a função não
    // existir no script carregado, ou o global encontrado não for do tipo
    // function, retorna false com uma mensagem controlada em getLastError(),
    // sem interromper o programa (docs/contracts.md, seções 4 e 10).
    bool callFunction(const std::string& functionName);

    const std::string& getLastError() const noexcept;
    lua_State* getState() noexcept;
    const lua_State* getState() const noexcept;

private:
    lua_State* state_;
    std::string lastError_;
};
