#pragma once

extern "C" {
#include <lua.h>
}

// Funções nativas em C++ expostas para os scripts Lua (docs/contracts.md, seção 9).
class LuaBindings {
public:
    // Registra todas as funções nativas no estado Lua informado.
    static void registrar(lua_State* L);

    // Atualiza o turno associado a este estado Lua. ArenaManager chama esta
    // função imediatamente antes do hook de início de turno.
    static void definir_turno_atual(lua_State* L, int turno);

private:
    // game_log(mensagem): encaminha uma string ao log do motor.
    // Não altera o estado da batalha. Argumentos inválidos geram erro
    // controlado via luaL_error, sem encerrar o programa.
    static int game_log(lua_State* L);

    // obter_turno_atual(): retorna o turno armazenado no registry do estado.
    static int obter_turno_atual(lua_State* L);
};
