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

Os testes automatizados ficam nas suítes de arenas, motor e integração Lua,
além do teste shell do fluxo principal. Para compilar o jogo e executar tudo:

```bash
make test
```

Isso gera e executa `build/arena-system-test`, `build/game-engine-test`,
`build/lua-integration-test` e `tests/main_flow_test.sh`. Cada caso imprime seu
resultado; qualquer falha encerra `make test` com código diferente de zero.

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

### O que `game_engine_test.cpp` cobre

| Caso | O que valida |
|---|---|
| Dano e defesa | Redução pela defesa, rejeição de valores inválidos e limite mínimo de vida |
| Cura | Limite na vida máxima e impossibilidade de reviver um personagem derrotado |
| Energia | Consulta de saldo, consumo validado e recuperação até o valor máximo |
| Queimadura e veneno | Aplicação, coexistência, reaplicação, duração, dano por turno e expiração |
| Sequência de turnos | Alternância jogador/inimigo e incremento da rodada depois do inimigo |
| Vitória e derrota | Resultado da batalha e bloqueio do avanço de turno após o encerramento |
| Menu de ações | Ataque básico, habilidades, entradas inválidas, EOF e configuração duplicada/vazia |

### O que os testes de integração e fluxo cobrem

| Suíte | O que valida |
|---|---|
| `lua_integration_test.cpp` | Conversão de personagens, contrato `ActionResult`, erros de `escolher_acao`, validação contextual e diferença real entre os dois scripts de inimigo |
| `main_flow_test.sh` | EOF encerra o executável sem avançar turno nem declarar vencedor |

## 4. Testes manuais de compilação

```bash
make clean && make build
```

**Resultado esperado:** `build/lua-arena` gerado sem warnings (o projeto
compila com `-Wall -Wextra`).

## 5. Testes manuais de execução — caminho feliz

1. **Goblin básico até o fim da batalha**

   ```bash
   ./build/lua-arena scripts/enemies/goblin_basic.lua
   ```

   Confirmar que a batalha roda até o fim (vitória ou derrota do
   jogador), sem travar e sem encerrar com erro.

2. **Goblin agressivo — mudança de comportamento**

   ```bash
   ./build/lua-arena scripts/enemies/goblin_aggressive.lua
   ```

   Confirmar que o inimigo passa a usar o golpe de acabamento quando o
   jogador fica com menos da metade da vida, sem alterar nem recompilar C++.

3. **Troca de arena sem recompilar**

   ```bash
   sha256sum build/lua-arena > /tmp/hash_antes.txt
   ./build/lua-arena scripts/enemies/goblin_basic.lua --arena scripts/arenas/volcanic.lua
   sha256sum build/lua-arena > /tmp/hash_depois.txt
   diff /tmp/hash_antes.txt /tmp/hash_depois.txt
   ```

   Confirmar que `diff` não aponta diferença (o binário é o mesmo antes
   e depois) e que o comportamento da Arena Vulcânica (dano de fogo
   amplificado, onda de calor periódica — ver
   [`docs/arenas-e-eventos.md`](arenas-e-eventos.md)) aparece na partida.

   A flag também pode ser combinada com
   `--difficulty scripts/difficulty/normal.lua`.

## 6. Testes manuais de tratamento de erro

Cada cenário abaixo tem um teste automatizado equivalente, rodável via
`make test`. Os comandos de CLI mostram como reproduzir os casos fim a fim.

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
printf '1\n' | ./build/lua-arena tests/fixtures/missing_action_function.lua
```

**Resultado esperado:** mensagem indicando que a função Lua não foi
encontrada (`"função Lua não encontrada: 'escolher_acao'"`), ação inimiga
recusada e execução encerrada sem crash quando a entrada terminar.

**Equivalente automatizado:** `testLuaEngineMissingFunction`.

### 6.4 Função retorna ação inválida

```bash
printf '1\n' | ./build/lua-arena tests/fixtures/invalid_enemy_action.lua
```

**Resultado esperado:** mensagem citando o campo obrigatório `valor`, turno
inimigo perdido, nenhum efeito aplicado e encerramento sem crash.

**Equivalente automatizado:** `testLuaEngineActionResultValidation` e os casos
de retorno inválido de `lua_integration_test.cpp`, que também cobrem retorno
não-table, campo ausente e tipo incorreto.

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
- [ ] Goblin agressivo demonstra comportamento diferente do básico.
- [ ] Troca de arena (`--arena scripts/arenas/volcanic.lua`) não altera o
      hash do binário e muda o comportamento da partida.
- [ ] Script `.lua` inexistente: erro claro, sem crash.
- [ ] Script com erro de sintaxe: erro claro, sem crash.
- [ ] Script sem `escolher_acao`: erro claro, sem crash.
- [ ] Ação inimiga sem campo obrigatório: erro claro, sem crash.
- [ ] Casos automatizados de retorno não-table, campo ausente e tipo incorreto
      passam em `make test`.
