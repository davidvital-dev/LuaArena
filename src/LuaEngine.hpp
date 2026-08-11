#pragma once

#include <string>

#include "ActionResult.hpp"

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

    // Chama uma função global sem argumentos. Se a função não existir no
    // script carregado, ou o global encontrado não for do tipo function,
    // retorna false com uma mensagem controlada em getLastError(), sem
    // interromper o programa (docs/contracts.md, seções 4 e 10). Quando
    // resultCount > 0, os retornos ficam empilhados para o chamador.
    bool callFunction(const std::string& functionName, int resultCount = 0);

    // Chama uma função Lua sem argumentos esperando 1 retorno, e valida e
    // converte esse retorno para ActionResult. Em qualquer falha (função
    // ausente, tipo errado, retorno que não é table, campo obrigatório
    // ausente/inválido), retorna false com mensagem específica em
    // getLastError() e não altera `result` (docs/contracts.md, seções 3, 4
    // e 10).
    bool callActionFunction(const std::string& functionName, ActionResult& result);

    const std::string& getLastError() const noexcept;
    lua_State* getState() noexcept;
    const lua_State* getState() const noexcept;

private:
    lua_State* state_;
    std::string lastError_;
};
