// Harness mínimo: teste para provar que troca o script de inimigo passado por linha de comando muda o comportamento de escolher_acao() usando o mesmo binário, sem recompilar o C++.
#include "Character.hpp"
#include "LuaEngine.hpp"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "uso: " << argv[0] << " <script-de-inimigo.lua>\n";
        return 1;
    }

    const std::string scriptPath = argv[1];

    LuaEngine engine;
    if (!engine.isInitialized()) {
        std::cerr << "falha ao inicializar o estado Lua\n";
        return 1;
    }

    if (!engine.loadScript(scriptPath)) {
        std::cerr << "falha ao carregar script: " << engine.getLastError() << '\n';
        return 1;
    }

    // Dados fixos só para exercitar escolher_acao(inimigo, jogador).
    const Character enemy{"Goblin", 60.0, 12.0, 3.0, 0.0};
    const Character player{"Herói", 100.0, 40.0, 5.0, 50.0};

    lua_State* state = engine.getState();

    lua_getglobal(state, "escolher_acao");
    if (!lua_isfunction(state, -1)) {
        std::cerr << "script não define escolher_acao\n";
        return 1;
    }

    if (!engine.pushCharacter(enemy) || !engine.pushCharacter(player)) {
        std::cerr << "falha ao montar tabelas de personagem: "
                  << engine.getLastError() << '\n';
        return 1;
    }

    if (lua_pcall(state, 2, 1, 0) != LUA_OK) {
        const char* message = lua_tostring(state, -1);
        std::cerr << "erro ao chamar escolher_acao: "
                  << (message == nullptr ? "erro Lua desconhecido" : message)
                  << '\n';
        return 1;
    }

    if (!lua_istable(state, -1)) {
        std::cerr << "escolher_acao não retornou uma table\n";
        return 1;
    }

    lua_getfield(state, -1, "tipo");
    lua_getfield(state, -2, "valor");
    lua_getfield(state, -3, "mensagem");

    const char* type = lua_tostring(state, -3);
    const double value = lua_tonumber(state, -2);
    const char* message = lua_tostring(state, -1);

    std::cout << "script: " << scriptPath << '\n';
    std::cout << "  tipo=" << (type == nullptr ? "?" : type)
              << " valor=" << value
              << " mensagem=\"" << (message == nullptr ? "" : message) << "\"\n";

    lua_pop(state, 4);  // mensagem, valor, tipo, tabela de retorno
    return 0;
}
