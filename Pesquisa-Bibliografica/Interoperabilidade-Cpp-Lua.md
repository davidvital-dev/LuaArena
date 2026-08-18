# Interoperabilidade C++ ↔ Lua no Lua Arena

## 1. Por que C++ e Lua, e não Python

O requisito pede exemplos de interoperabilidade, quando tecnicamente viável, com Python ou outras linguagens. O Lua Arena não usa Python em nenhum ponto do motor: a interoperabilidade que já existe, testada e em produção no projeto, é entre C++ (hospedeiro) e Lua (linguagem de extensão), descrita conceitualmente em [`contextualizacao.md`](contextualizacao.md) e formalizada em [`contracts.md`](../docs/contracts.md). Este documento usa código real do repositório para mostrar essa interoperabilidade nas duas direções, em vez de um exemplo artificial com uma linguagem que o projeto não integra.

A comunicação entre C++ e Lua acontece pela API C oficial de Lua, que troca valores por meio de uma stack virtual [LUA-MANUAL]. Não existe serialização em texto (como JSON) nem processos separados: o interpretador Lua roda embutido dentro do mesmo processo C++, e os dois lados trocam valores diretamente pela stack.

## 2. Direção C++ → Lua: chamar uma função Lua a partir do motor

Quando o motor precisa que um script decida o efeito de uma habilidade, `LuaEngine::callAbilityFunction` (`src/LuaEngine.cpp`) empilha o nome da habilidade e os dados dos dois personagens, chama a função Lua `usar_habilidade` e lê o resultado de volta:

```cpp
// src/LuaEngine.cpp
rawGetGlobal(state_, "usar_habilidade");
// ...
lua_pushlstring(state_, abilityName.data(), abilityName.size());
pushCharacter(player);   // cria uma table Lua com os atributos do personagem
pushCharacter(enemy);

lua_pcall(state_, 3, 1, 0);   // 3 argumentos, 1 retorno esperado
```

`pushCharacter` converte um `Character` C++ em uma table Lua com campos como `vida`, `ataque` e `energia` — não há passagem de ponteiros ou objetos C++ para o script, só valores simples (`lua_createtable`, `lua_pushnumber`, `lua_pushstring`).

Do lado Lua, `usar_habilidade` (`scripts/abilities/abilities.lua`) recebe esses três valores como argumentos normais de função:

```lua
-- scripts/abilities/abilities.lua
function usar_habilidade(nome, jogador, inimigo)
    -- jogador.energia, jogador.nome etc. vêm da table criada por pushCharacter
    if energia < habilidade.custo then
        return falha("Energia insuficiente.")
    end
    return {
        sucesso = true,
        tipo = "habilidade",
        custo = habilidade.custo,
        dano = habilidade.dano,
        -- ...
    }
end
```

A table retornada volta para o topo da stack, e o C++ a lê campo a campo (`callAbilityFunction`), validando tipos e limites antes de aplicar qualquer efeito ao estado real do jogo. Esse ponto — validar tudo que volta do script antes de usar — é o mesmo princípio de fronteira de confiança discutido em [`contextualizacao.md`](contextualizacao.md).

## 3. Direção Lua → C++: chamar uma função C++ a partir do script

A outra direção também existe: funções C++ registradas como funções Lua, chamáveis diretamente pelos scripts. `LuaBindings::registrar` (`src/LuaBindings.cpp`) expõe duas delas ao estado Lua:

```cpp
// src/LuaBindings.cpp
void LuaBindings::registrar(lua_State* L) {
    lua_register(L, "game_log", LuaBindings::game_log);
    lua_register(L, "obter_turno_atual", LuaBindings::obter_turno_atual);
}

int LuaBindings::game_log(lua_State* L) {
    // exige exatamente 1 argumento string
    const char* mensagem = lua_tostring(L, 1);
    std::cout << "[game_log] " << mensagem << std::endl;
    return 0;
}
```

`lua_register` associa o nome `game_log` a uma função C que segue a assinatura exigida pela API Lua (`int (*)(lua_State*)`): ela lê seus argumentos e retornos pela mesma stack usada na direção anterior [LUA-MANUAL]. A partir daí, qualquer script Lua carregado nesse estado pode chamar `game_log("mensagem")` como se fosse uma função Lua nativa — o script não sabe, nem precisa saber, que a implementação está em C++.

`obter_turno_atual` segue o mesmo mecanismo, mas no sentido inverso de dados: em vez de receber um argumento, ela lê um valor guardado pelo C++ no registry de Lua (`LUA_REGISTRYINDEX`) e o devolve ao script que a chamou, permitindo que o Lua leia um estado controlado pelo motor sem que esse estado vire uma variável global Lua comum.

## 4. Por que essa integração conta como interoperabilidade

As duas direções acima cumprem a definição do requisito: comunicação efetiva entre duas tecnologias diferentes, com dados passando de um lado para o outro em tempo de execução, não apenas documentação comparativa (esse papel já é cumprido por [`comparacao-lua-python.md`](comparacao-lua-python.md)). Diferente de um exemplo isolado criado só para a pesquisa, este é o mecanismo de interoperabilidade real que o Lua Arena usa em cada turno de batalha, coberto pelos testes em [`tests/lua_integration_test.cpp`](../tests/lua_integration_test.cpp).

As referências identificadas entre colchetes estão descritas em [`referencias.md`](referencias.md).
