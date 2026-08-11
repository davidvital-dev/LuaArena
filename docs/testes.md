# Roteiro de testes — Lua Arena

## 1. Objetivo

Este roteiro cobre a validação da integração entre o motor em C++ e os
scripts Lua do projeto. O objetivo central é garantir duas coisas:

1. **A integração funciona**: scripts Lua válidos são carregados,
   executados e seus retornos convertidos corretamente para as estruturas
   do motor (`ActionResult`, `ArenaEvent`, `DifficultyConfig`, `ArenaConfig`).
2. **Os erros são tratados sem derrubar o programa**: script inexistente,
   erro de sintaxe, função ausente e retorno em formato inválido devem
   sempre produzir uma mensagem de erro clara, preservar o estado do
   motor e permitir que a execução continue — nunca um crash.

Este documento resolve as issues **#54** e **#63** ("Criar roteiro de
testes do projeto").

## 2. Pré-requisitos

Antes de qualquer teste, confirme que o ambiente tem as dependências
necessárias:

```bash
make check-deps
```

Esse alvo valida `g++`, `pkg-config` e a biblioteca de desenvolvimento da
Lua 5.4. Só prossiga com o restante do roteiro depois que ele imprimir
`Todas as dependências estão OK.`.

## 3. Testes automatizados

Os testes automatizados ficam em `tests/arena_system_test.cpp` e cobrem o
motor de arenas, dificuldade e o `LuaEngine`. Para compilar e rodar:

```bash
make test
```

Isso gera `build/arena-system-test` e o executa. Cada caso de teste
imprime `[ok] <nome>` em caso de sucesso; qualquer falha interrompe a
execução com `[falha] <mensagem>` e código de saída `1`.

### O que `arena_system_test.cpp` cobre hoje

| Caso | O que valida |
|---|---|
| Carregamento de dificuldade | `scripts/difficulty/normal.lua` e defaults seguros para campos inválidos |
| Leitura raw de contratos Lua | Proteção contra metatables/proxies maliciosos em tabelas de configuração |
| Configurações e troca de arena | As quatro arenas obrigatórias carregam e trocam sem recompilar (`ArenaManager::load`) |
| Hooks opcionais e ciclo de vida | `ao_iniciar_batalha`, `ao_iniciar_turno`, `ao_finalizar_batalha` ausentes não geram erro |
| Eventos periódicos | Eventos de arena (onda de calor, veneno, cura) aplicados corretamente por turno |
| Dados Lua inválidos | Configuração de arena e eventos malformados são rejeitados com erro controlado |
| Script Lua inexistente | `LuaEngine::loadScript` com caminho inexistente |
| Script Lua com erro de sintaxe | `LuaEngine::loadScript` com script malformado |
| Função Lua ausente | `LuaEngine::callFunction` com função inexistente ou global de outro tipo |
| Validação de retorno `ActionResult` | `LuaEngine::callActionFunction` com retorno que não é table, campos obrigatórios ausentes/inválidos e campos opcionais inválidos |
| Integração com habilidades | `tests/abilities_test.lua`, cobrindo `usar_habilidade` do catálogo real |

Cada um dos quatro últimos casos é a base para os testes manuais
equivalentes da seção 6 — eles já garantem, hoje, que o comportamento
descrito ali está implementado e testado no nível de `LuaEngine`.

## 4. Testes manuais de compilação

```bash
make clean && make build
```

**Resultado esperado:** `build/lua-arena` gerado sem warnings (o projeto
compila com `-Wall -Wextra`).

> **Status atual:** este alvo depende de `src/main.cpp`, que ainda não
> existe no repositório (a integração final do loop jogável está em
> andamento — ver seção "Status" do [`README.md`](../README.md)). Até
> `main.cpp` ser adicionado, `make build` falha apenas na etapa de
> *link*, com `undefined reference to 'main'`; todos os `.o` individuais
> compilam normalmente. Use `make test` (seção 3) para validar a lógica
> do motor enquanto isso.

## 5. Testes manuais de execução — caminho feliz

> **Bloqueado:** os três itens abaixo dependem de `src/main.cpp` (com
> leitura de argumentos de linha de comando) e dos scripts de inimigo
> `scripts/enemies/goblin_basic.lua` e `scripts/enemies/goblin_smart.lua`,
> nenhum dos quais existe hoje no repositório (`scripts/enemies/` só tem
> `.gitkeep`). Documentado aqui como o roteiro a seguir assim que essas
> peças forem integradas.

1. **Goblin básico até o fim da batalha**

   ```bash
   ./build/lua-arena scripts/enemies/goblin_basic.lua
   ```

   Confirmar que a batalha roda até o fim (vitória ou derrota do
   jogador), sem travar e sem encerrar com erro.

2. **Goblin inteligente — mudança de comportamento**

   ```bash
   ./build/lua-arena scripts/enemies/goblin_smart.lua
   ```

   Confirmar que as decisões do inimigo (ex: curar quando a vida está
   baixa) são visivelmente diferentes do goblin básico, sem que nenhuma
   linha de C++ tenha sido alterada.

3. **Troca de arena sem recompilar**

   ```bash
   md5sum build/lua-arena > /tmp/hash_antes.txt
   ./build/lua-arena scripts/enemies/goblin_basic.lua --arena scripts/arenas/volcanic.lua
   md5sum build/lua-arena > /tmp/hash_depois.txt
   diff /tmp/hash_antes.txt /tmp/hash_depois.txt
   ```

   Confirmar que `diff` não aponta diferença (o binário é o mesmo antes
   e depois) e que o comportamento da Arena Vulcânica (dano de fogo
   amplificado, onda de calor periódica — ver
   [`docs/arenas-e-eventos.md`](arenas-e-eventos.md)) aparece na partida.

   > A flag `--arena` ainda não tem contrato definido em nenhum
   > documento do projeto; ela é proposta aqui como a interface mais
   > simples compatível com o alvo `run` do `Makefile`. Quem integrar
   > `main.cpp` deve confirmar ou ajustar esse formato.

## 6. Testes manuais de tratamento de erro

Cada cenário abaixo já tem um teste automatizado equivalente no nível de
`LuaEngine` (seção 3), rodável hoje via `make test`. Os comandos de CLI
abaixo mostram como reproduzir o mesmo cenário fim-a-fim, e dependem do
`main.cpp` descrito na seção 5.

### 6.1 Arquivo `.lua` inexistente

```bash
./build/lua-arena scripts/enemies/nao_existe.lua
```

**Resultado esperado:** mensagem clara indicando que o script não foi
encontrado (`LuaEngine::loadScript` retorna
`"script Lua não encontrado: '<caminho>'"`), programa encerra com código
de saída diferente de zero, sem crash.

**Equivalente automatizado:** `testLuaEngineMissingScript` em
`tests/arena_system_test.cpp`.

### 6.2 Script com erro de sintaxe

Crie um script com uma vírgula ou `end` faltando, por exemplo
(veja `tests/fixtures/syntax_error.lua`):

```lua
function ao_iniciar_batalha(jogador, inimigo)
    return nil
-- 'end' ausente de propósito
```

```bash
./build/lua-arena tests/fixtures/syntax_error.lua
```

**Resultado esperado:** mensagem de erro do parser Lua propagada com
arquivo e linha do problema, sem crash, código de saída diferente de
zero.

**Equivalente automatizado:** `testLuaEngineSyntaxError`.

### 6.3 Script sem a função esperada

Use um script válido que não define `escolher_acao` (veja
`tests/fixtures/missing_action_function.lua`):

```bash
./build/lua-arena tests/fixtures/missing_action_function.lua
```

**Resultado esperado:** mensagem indicando que a função Lua não foi
encontrada (`"função Lua não encontrada: 'escolher_acao'"`), sem crash,
código de saída diferente de zero.

**Equivalente automatizado:** `testLuaEngineMissingFunction`.

### 6.4 Função retorna valor inválido

Três variações, todas cobertas por `tests/fixtures/action_result_functions.lua`:

| Retorno da função Lua | Resultado esperado |
|---|---|
| Uma `string` em vez de uma tabela | `"retorno de '<nome>' deve ser table, recebido: string"` |
| Tabela sem o campo `tipo` | `"campo 'tipo' ausente ou não é string no retorno de '<nome>'"` |
| Campo com tipo errado (ex: `valor = "dez"`) | `"campo 'valor' ausente ou não é number no retorno de '<nome>'"` |

```bash
./build/lua-arena tests/fixtures/action_result_functions.lua
```

**Resultado esperado, para os três casos:** mensagem específica citando
o campo problemático, sem crash, código de saída diferente de zero, e o
motor não aplica nenhum efeito com dados inválidos.

**Equivalente automatizado:** `testLuaEngineActionResultValidation`.

## 7. Critério de sucesso da demonstração

A regra arquitetural do projeto (ver [`README.md`](../README.md)) é:

```text
Lua decide ou descreve.
C++ valida e aplica.
```

Na prática, isso significa que a demonstração é bem-sucedida quando:

- **Só os arquivos `.lua` mudam entre execuções** — trocar de inimigo,
  arena ou dificuldade não exige alterar nenhum arquivo `.cpp`/`.hpp`.
- **O binário C++ não é recompilado** entre uma execução e outra (seção
  5.3 comprova isso comparando o hash do executável).
- **O comportamento do jogo muda mesmo assim** — inimigo mais agressivo,
  arena com modificadores diferentes, dificuldade mais alta — apenas por
  causa do conteúdo do script Lua carregado.
- **Nenhum erro de script derruba o programa** — todos os cenários da
  seção 6 terminam com mensagem clara e código de saída não-zero, nunca
  com um crash ou traço de pilha do sistema operacional.

## 8. Checklist final

Use esta lista durante a apresentação/demonstração ao vivo.

- [ ] `make check-deps` confirma todas as dependências.
- [ ] `make test` roda e todos os casos imprimem `[ok]`.
- [ ] `make clean && make build` gera `build/lua-arena` sem warnings.
- [ ] Goblin básico joga uma batalha completa até vitória/derrota.
- [ ] Goblin inteligente demonstra comportamento diferente do básico.
- [ ] Troca de arena (`--arena scripts/arenas/volcanic.lua`) não altera o
      hash do binário e muda o comportamento da partida.
- [ ] Script `.lua` inexistente: erro claro, sem crash.
- [ ] Script com erro de sintaxe: erro claro, sem crash.
- [ ] Script sem `escolher_acao`: erro claro, sem crash.
- [ ] Função Lua retorna valor não-table: erro claro, sem crash.
- [ ] Função Lua retorna table sem campo `tipo`: erro claro, sem crash.
- [ ] Função Lua retorna campo com tipo errado (ex: `valor` como
      string): erro claro, sem crash.
