# Build, bindings e execução — Lua Arena

Este documento aprofunda o que o [`README.md`](../README.md) apresenta de
forma resumida: como compilar o projeto, como a comunicação Lua → C++
("bindings") funciona internamente, como os erros de script são tratados
e como rodar o jogo. Resolve as issues **#61** e **#70** ("Documentar
bindings, testes e execução").

## 1. Build

### Dependências

- `g++` com suporte a C++17;
- `pkg-config`;
- biblioteca de desenvolvimento da Lua 5.4 (`sudo apt install liblua5.4-dev`
  em distribuições baseadas em Debian/Ubuntu).

### Alvos do Makefile

| Alvo | O que faz | Exemplo de uso |
|---|---|---|
| `check-deps` | Confirma que `g++`, `pkg-config` e a Lua 5.4 estão instalados via `pkg-config` (tenta `lua5.4`, depois `lua-5.4`, depois `lua`) | `make check-deps` |
| `build` | Compila todos os `.cpp` de `src/` e gera o binário em `build/lua-arena` | `make build` |
| `run` | Compila (se necessário) e executa o jogo | `make run` |
| `test` | Compila e executa a suíte de testes automatizados (`tests/arena_system_test.cpp`) | `make test` |
| `clean` | Remove o diretório `build/` inteiro | `make clean` |
| `rebuild` | Equivale a `make clean && make build` | `make rebuild` |

O alvo `build` descobre os arquivos-fonte automaticamente
(`$(wildcard src/*.cpp)`), então novos arquivos `.cpp` em `src/` entram na
compilação sem precisar editar o `Makefile`.

### Se `make check-deps` falhar

O alvo aponta exatamente qual dependência está faltando (compilador,
`pkg-config` ou biblioteca Lua). Consulte a seção
[Dependências do `README.md`](../README.md#dependências) para o pacote
exato a instalar e rode `make check-deps` novamente até ele confirmar
`Todas as dependências estão OK.`.

## 2. Bindings (Lua chamando C++)

### O que é um "binding" neste projeto

Um *binding* é uma função escrita em C++ que fica visível como função
global dentro dos scripts Lua. Ela é o único canal pelo qual um script
consegue pedir algo ao motor sem que o motor precise expor seu estado
interno — reforçando a regra do projeto (*"Lua decide ou descreve, C++
valida e aplica"*, ver `docs/contracts.md`). Hoje, todos os bindings do
projeto vivem na classe `LuaBindings` (`src/LuaBindings.hpp`/`.cpp`).

### Como o registro funciona

```cpp
// src/LuaBindings.hpp
class LuaBindings {
public:
    static void registrar(lua_State* L);
    static void definir_turno_atual(lua_State* L, int turno);

private:
    static int game_log(lua_State* L);
    static int obter_turno_atual(lua_State* L);
};
```

`LuaBindings::registrar(L)` é chamada uma vez para cada `lua_State` criado
pelo motor, logo depois de `luaL_openlibs`. Ela usa `lua_register`, que é
apenas um atalho de `lauxlib.h` para "criar uma global com este nome
apontando para esta função C":

```cpp
// src/LuaBindings.cpp
void LuaBindings::registrar(lua_State* L) {
    lua_register(L, "game_log", LuaBindings::game_log);
    lua_register(L, "obter_turno_atual", LuaBindings::obter_turno_atual);
    definir_turno_atual(L, 0);
}
```

Cada função exposta precisa ter a assinatura exigida pela API C da Lua
(`int (*)(lua_State*)`) — é por isso que `game_log` e
`obter_turno_atual` são `static int nome(lua_State* L)`, e não métodos
comuns.

Na prática, `registrar` é chamado dentro de `ArenaManager::load`, logo
após abrir as bibliotecas padrão do novo estado Lua:

```cpp
// src/ArenaManager.cpp
luaL_openlibs(candidate);
LuaBindings::registrar(candidate);
```

Isso garante que qualquer script de arena, inimigo, habilidade ou
dificuldade carregado através de um `LuaEngine`/`ArenaManager` tem acesso
aos bindings — sem precisar registrá-los manualmente em cada lugar.

### `game_log` em detalhe

Assinatura no Lua:

```lua
game_log("O Goblin entrou em modo de desespero.")
```

Contrato (ver também `docs/contracts.md`, seção 9): recebe **exatamente
um argumento do tipo `string`**, não altera o estado da batalha, apenas
encaminha a mensagem para o log do motor (hoje, `std::cout`).

Implementação:

```cpp
// src/LuaBindings.cpp
int LuaBindings::game_log(lua_State* L) {
    int argc = lua_gettop(L);

    if (argc != 1 || lua_type(L, 1) != LUA_TSTRING) {
        return luaL_error(L, "game_log espera exatamente um argumento do tipo string.");
    }

    const char* mensagem = lua_tostring(L, 1);
    std::cout << "[game_log] " << mensagem << std::endl;

    return 0;
}
```

Comportamento em caso de uso incorreto:

| Chamada Lua | Resultado |
|---|---|
| `game_log("ok")` | Imprime `[game_log] ok`, retorna normalmente |
| `game_log(123)` | Erro controlado via `luaL_error`: `"game_log espera exatamente um argumento do tipo string."` |
| `game_log("a", "b")` | Mesmo erro acima (número de argumentos incorreto) |
| `game_log()` | Mesmo erro acima (nenhum argumento) |

`luaL_error` levanta um erro Lua normal — ele é capturado por quem chamou
o script via `lua_pcall` (por exemplo `LuaEngine::loadScript` ou
`ArenaManager::callHook`), então uma chamada inválida a `game_log` nunca
derruba o programa: o pior caso é a execução do *hook* atual ser
interrompida e o erro aparecer em `lastError()`.

### Passo a passo: adicionando um novo binding

Para expor uma nova função C++ para os scripts Lua, siga o padrão de
`game_log`:

1. **Declare** o método em `src/LuaBindings.hpp`, como `static int` privado,
   com a assinatura `int (lua_State* L)`:

   ```cpp
   static int minha_funcao(lua_State* L);
   ```

2. **Implemente** em `src/LuaBindings.cpp`. Valide sempre a quantidade e o
   tipo de cada argumento antes de usá-los (`lua_gettop`, `lua_type`), e
   use `luaL_error` para reportar uso incorreto sem interromper o
   programa:

   ```cpp
   int LuaBindings::minha_funcao(lua_State* L) {
       if (lua_gettop(L) != 1 || lua_type(L, 1) != LUA_TNUMBER) {
           return luaL_error(L, "minha_funcao espera exatamente um argumento number.");
       }

       double valor = lua_tonumber(L, 1);
       // ... lógica da função ...

       lua_pushnumber(L, valor * 2);
       return 1;  // quantidade de valores retornados para o Lua
   }
   ```

3. **Registre** a função dentro de `LuaBindings::registrar`, junto às
   demais:

   ```cpp
   lua_register(L, "minha_funcao", LuaBindings::minha_funcao);
   ```

4. **Documente o contrato** em `docs/contracts.md` (seção 9), incluindo
   assinatura, validação e comportamento em caso de erro — seguindo o
   mesmo formato usado para `game_log`.

5. **Cubra com teste**: adicione um caso em `tests/arena_system_test.cpp`
   (ou um novo arquivo de teste, se fizer mais sentido) chamando a
   função a partir de um script Lua de fixture, cobrindo o caminho feliz
   e pelo menos um caso de erro.

## 3. Tratamento de erros de script

`LuaEngine` (`src/LuaEngine.cpp`) trata quatro cenários de erro de forma
controlada: nunca lança exceção nem derruba o programa, sempre limpa a
stack Lua e sempre preenche `lastError()` com uma mensagem específica.

### Arquivo inexistente

```bash
# nenhum script Lua real neste caminho
```

```cpp
engine.loadScript("scripts/enemies/nao_existe.lua");
```

Mensagem: `"script Lua não encontrado: 'scripts/enemies/nao_existe.lua'"`.
Verificado antes de qualquer chamada à Lua, via
`std::filesystem::is_regular_file`.

### Erro de sintaxe

```lua
-- tests/fixtures/syntax_error.lua
function ao_iniciar_batalha(jogador, inimigo)
    return nil
-- 'end' ausente de propósito
```

Mensagem: erro do parser da Lua propagado com arquivo e linha, por
exemplo `"falha ao carregar script '...': ...:4: 'end' expected ..."`.

### Função Lua ausente

```lua
-- script válido, mas sem escolher_acao
function preparar_inimigo()
    -- ...
end
```

```cpp
engine.callFunction("escolher_acao");
```

Mensagem: `"função Lua não encontrada: 'escolher_acao'"`. Se o nome
existir mas não for uma função (por exemplo, uma variável global comum),
a mensagem é `"'<nome>' existe, mas não é uma função Lua"`.

### Retorno Lua inválido

```lua
function acao_invalida()
    return "isso não é uma tabela"
end
```

```cpp
ActionResult result;
engine.callActionFunction("acao_invalida", result);
```

Mensagem: `"retorno de 'acao_invalida' deve ser table, recebido: string"`.
Campos obrigatórios ausentes ou com tipo errado (`tipo`, `valor`) e
campos opcionais presentes com tipo errado (`mensagem`, `efeito`,
`duracao`, `custo`) geram mensagens específicas citando o campo, por
exemplo `"campo 'valor' ausente ou não é number no retorno de '...'"`.

**Em todos os quatro casos**, o programa continua utilizável: o
`LuaEngine` pode carregar ou chamar outro script/função normalmente logo
em seguida, sem precisar ser recriado.

## 4. Execução

Comando básico esperado, seguindo o alvo `run` do `Makefile`:

```bash
./build/lua-arena scripts/enemies/goblin_basic.lua
```

> **Status atual:** a execução completa do jogo (binário `main.cpp`,
> scripts `goblin_basic.lua`/`goblin_smart.lua` e a leitura de
> argumentos de linha de comando para trocar inimigo, dificuldade e
> arena) ainda está em integração — ver a seção "Status" do
> [`README.md`](../README.md) e o detalhamento em
> [`docs/testes.md`](testes.md#5-testes-manuais-de-execução--caminho-feliz).
> Até lá, o comportamento de cada peça (bindings, carregamento de
> script, arenas, dificuldade, habilidades) já é validado
> individualmente pela suíte de testes da seção 5.

Quando a integração estiver pronta, a expectativa é trocar de inimigo
apontando para outro arquivo em `scripts/enemies/`, e de arena/dificuldade
por meio de flags de linha de comando (proposta em
`docs/testes.md`, seção 5.3, como `--arena <caminho>`), sem recompilar o
binário.

## 5. Testes

O roteiro completo de testes — automatizados e manuais, incluindo cada
cenário de erro reproduzido via linha de comando — está em
[`docs/testes.md`](testes.md). Em resumo: `make test` roda a suíte
automatizada que cobre dificuldade, arenas, eventos e os quatro cenários
de erro do `LuaEngine` descritos na seção 3 deste documento; os testes
manuais complementam com o caminho feliz de execução e um checklist para
demonstração ao vivo.

## 6. Referências cruzadas

- [`README.md`](../README.md) — visão geral do projeto, estrutura de
  diretórios e comandos de build.
- [`docs/contracts.md`](contracts.md) — contratos completos entre C++ e
  Lua, incluindo a seção 9 sobre funções nativas acessíveis por Lua.
- [`docs/testes.md`](testes.md) — roteiro completo de testes automatizados
  e manuais.
