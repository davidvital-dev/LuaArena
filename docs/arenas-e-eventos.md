# Arenas e eventos

Este guia descreve como carregar arenas e dificuldades, conectar o ciclo de
batalha e criar novos scripts sem recompilar o jogo.

## Visão geral

O fluxo mantém a regra arquitetural do projeto:

1. Lua declara a configuração e descreve um evento.
2. `ArenaManager` executa o hook protegido e valida seu retorno.
3. O motor C++ aplica dano, cura, defesa e efeitos no estado real.

Lua recebe apenas cópias dos dados dos personagens. Alterar essas tabelas não
altera a vida, a energia ou qualquer outro estado mantido pelo C++.

## Carregamento

```cpp
#include "ArenaManager.hpp"

ArenaManager arenas;
if (!arenas.load("scripts/arenas/volcanic.lua")) {
    registrar_erro(arenas.lastError());
}

const ArenaConfig* config = arenas.config();
double fatorFogo = config->modifier(ArenaModifier::FireDamage);
```

Cada chamada bem-sucedida de `load` cria um estado Lua limpo e substitui a
arena anterior. Assim, basta informar outro arquivo para trocar de ambiente em
tempo de execução. Em caso de falha, a arena anterior permanece disponível.

O carregamento de dificuldade é equivalente:

```cpp
DifficultyLoader difficulty;
if (difficulty.load("scripts/difficulty/normal.lua")) {
    const DifficultyConfig& config = difficulty.config();
    aplicar_vida(config.healthMultiplier);
    aplicar_ataque(config.attackMultiplier);
}
```

Campos inválidos de dificuldade recebem defaults seguros e ficam disponíveis
em `warnings()`. Erro de script ou ausência de `configuracao` fica em
`lastError()`.

## Ciclo de vida

O loop do motor deve chamar os métodos nesta ordem:

```cpp
arenas.onBattleStart(jogador, inimigo);

for (int turno = 1; batalha_ativa; ++turno) {
    auto evento = arenas.onTurnStart(turno, jogador, inimigo);
    if (evento) {
        aplicar_evento_validado(*evento);
    }
}

arenas.onBattleEnd(BattleResult::Victory, jogador, inimigo);
```

`onTurnStart` não executa eventos antes do início ou depois do fim da batalha.
Os hooks são opcionais; a Arena Neutra, por exemplo, não declara nenhum deles.

Ao aplicar um evento validado, o motor ainda deve respeitar seus invariantes:

- dano nunca reduz a vida abaixo de zero;
- cura nunca ultrapassa `vida_maxima`;
- efeitos usam o catálogo do motor e expiram pela duração informada;
- nenhuma alteração é aplicada quando a batalha já terminou.

## Contrato de script

Uma arena declara nome, descrição e modificadores:

```lua
arena = {
    nome = "Exemplo",
    descricao = "Arena usada como modelo.",
    modificadores = {
        dano_fogo = 1.10,
        cura = 0.90
    }
}
```

Um hook retorna `nil` ou uma tabela:

```lua
function ao_iniciar_turno(turno, jogador, inimigo)
    if turno % 3 ~= 0 then
        return nil
    end

    return {
        alvo = "todos",
        tipo = "dano",
        valor = 5,
        mensagem = "O ambiente reage.",
        efeito = nil,
        duracao = 0
    }
end
```

### Campos de evento

| Campo | Obrigatório | Valores aceitos |
|---|---:|---|
| `alvo` | sim | `jogador`, `inimigo`, `todos` |
| `tipo` | sim | `dano`, `cura`, `defesa`, `nenhum` |
| `valor` | sim | número finito e não negativo |
| `mensagem` | sim | string não vazia de até 4096 bytes |
| `efeito` | não | `queimadura`, `veneno`, `defesa`, `nenhum` ou `nil` |
| `duracao` | não | inteiro não negativo; zero quando não há efeito |

Um efeito exige duração positiva. Um evento do tipo `nenhum` não pode carregar
valor diferente de zero nem efeito. Retornos inválidos são descartados.

## Arenas incluídas

| Arquivo | Modificador | Evento periódico |
|---|---|---|
| `neutral.lua` | nenhum | nenhum |
| `volcanic.lua` | dano de fogo × 1,25 | 5 de dano em todos a cada 3 turnos |
| `poison_forest.lua` | dano de veneno × 1,10 | 3 de dano e veneno por 2 turnos a cada 2 turnos |
| `healing_temple.lua` | cura × 1,20 | 8 de cura em todos a cada 3 turnos |

A Arena Vulcânica demonstra `obter_turno_atual()`, binding C++ que consulta o
turno registrado no estado Lua.

## Validação e testes

Instale as dependências descritas no README e execute:

```bash
make test
```

A suíte cobre:

- carregamento e troca das quatro arenas sem recompilação;
- hooks ausentes, erros de `lua_pcall` e recuperação da Lua Stack;
- alvo, tipo, valor, efeito, duração e campos obrigatórios inválidos;
- eventos vulcânicos, veneno e cura limitada à vida máxima;
- defaults da dificuldade;
- integração com o script de habilidades já existente.
