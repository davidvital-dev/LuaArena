# Contratos de integração entre C++ e Lua

Este documento define os formatos que devem ser seguidos na comunicação entre o motor em C++ e os scripts Lua do **Lua Arena**.

O objetivo é impedir que diferentes partes do projeto usem nomes de funções, campos ou tipos incompatíveis.

> **Regra central:** Lua decide ou descreve uma ação. C++ valida e aplica seus efeitos.

---

## 1. Responsabilidades de cada linguagem

### Lua

Lua será responsável por:

- decidir ações dos inimigos;
- descrever habilidades;
- fornecer configurações de dificuldade;
- fornecer configurações de arena;
- descrever eventos disparados pelas arenas;
- produzir mensagens relacionadas às decisões dos scripts.

### C++

C++ será responsável por:

- armazenar o estado real do jogo;
- controlar vida, energia, turnos, vitória e derrota;
- validar todos os valores recebidos de Lua;
- aplicar dano, cura, custos e efeitos de status;
- impedir ações inválidas;
- tratar erros de carregamento e execução dos scripts;
- manter a execução segura quando um script retornar dados incorretos.

Lua nunca deve alterar diretamente a vida, energia ou estado interno dos personagens.

---

## 2. Representação de personagem enviada para Lua

O C++ enviará jogador e inimigo como tabelas Lua.

```lua
personagem = {
    nome = "Herói",
    vida = 100,
    vida_maxima = 100,
    ataque = 20,
    defesa = 5,
    energia = 50,
    energia_maxima = 50
}
```

### Campos obrigatórios

| Campo | Tipo Lua | Descrição |
|---|---|---|
| `nome` | `string` | Nome exibido do personagem |
| `vida` | `number` | Vida atual |
| `vida_maxima` | `number` | Limite máximo de vida |
| `ataque` | `number` | Valor-base de ataque |
| `defesa` | `number` | Valor-base de defesa |
| `energia` | `number` | Energia atual |
| `energia_maxima` | `number` | Limite máximo de energia |

### Regras

- Os valores numéricos enviados pelo C++ não podem ser negativos.
- `vida` não pode superar `vida_maxima`.
- `energia` não pode superar `energia_maxima`.
- Os scripts devem apenas consultar esses dados.
- Alterações feitas diretamente na tabela Lua não modificam automaticamente o personagem em C++.

---

## 3. Resultado de uma ação

As decisões de inimigos e os eventos de arena serão convertidos pelo C++ para uma estrutura equivalente a:

```cpp
struct ActionResult {
    std::string type;
    double value;
    std::string message;
    std::string effect;
    int duration;
    double energyCost;
};
```

Em Lua, o formato padrão será:

```lua
{
    tipo = "ataque",
    valor = 15,
    mensagem = "O Goblin realizou um ataque normal.",
    efeito = nil,
    duracao = 0,
    custo = 0
}
```

### Campos

| Campo | Obrigatório | Tipo Lua | Descrição |
|---|---:|---|---|
| `tipo` | sim | `string` | Natureza da ação |
| `valor` | sim | `number` | Valor principal de dano, cura ou defesa |
| `mensagem` | sim | `string` | Texto exibido pelo motor |
| `efeito` | não | `string` ou `nil` | Efeito de status aplicado |
| `duracao` | não | `number` inteiro | Quantidade de turnos do efeito |
| `custo` | não | `number` | Energia necessária para executar a ação |

Quando ausentes, os campos opcionais serão interpretados como:

```text
efeito = nenhum
duracao = 0
custo = 0
```

### Tipos de ação permitidos

```text
ataque
cura
defesa
habilidade
nenhum
```

### Efeitos inicialmente permitidos

```text
queimadura
veneno
defesa
nenhum
```

O valor Lua `nil` também representa ausência de efeito.

---

## 4. Escolha de ação do inimigo

Todo script de inimigo deverá disponibilizar a função global:

```lua
escolher_acao(inimigo, jogador)
```

### Parâmetros

1. `inimigo`: tabela do personagem controlado pelo script;
2. `jogador`: tabela do jogador.

### Retorno

A função deve retornar uma única tabela no formato de `ActionResult`.

Exemplo:

```lua
function escolher_acao(inimigo, jogador)
    return {
        tipo = "ataque",
        valor = inimigo.ataque,
        mensagem = inimigo.nome .. " realizou um ataque normal.",
        efeito = nil,
        duracao = 0,
        custo = 0
    }
end
```

Exemplo com cura:

```lua
function escolher_acao(inimigo, jogador)
    if inimigo.vida < inimigo.vida_maxima * 0.30 then
        return {
            tipo = "cura",
            valor = 15,
            mensagem = inimigo.nome .. " recuperou parte da vida.",
            efeito = nil,
            duracao = 0,
            custo = 0
        }
    end

    return {
        tipo = "ataque",
        valor = inimigo.ataque,
        mensagem = inimigo.nome .. " atacou normalmente.",
        efeito = nil,
        duracao = 0,
        custo = 0
    }
end
```

### Comportamento em caso de falha

Se a função não existir, gerar erro, não retornar uma tabela ou retornar campos inválidos, o C++ deve:

1. registrar uma mensagem de erro;
2. limpar corretamente a Lua Stack;
3. impedir que o retorno inválido seja aplicado;
4. usar uma ação segura de fallback ou encerrar apenas o turno atual, conforme a decisão do motor;
5. manter o programa em execução sempre que possível.

---

## 5. Consulta e uso de habilidades

O script de habilidades deverá disponibilizar:

```lua
usar_habilidade(nome, jogador, inimigo)
```

### Parâmetros

| Parâmetro | Tipo | Descrição |
|---|---|---|
| `nome` | `string` | Identificador da habilidade |
| `jogador` | `table` | Dados atuais do jogador |
| `inimigo` | `table` | Dados atuais do inimigo |

As habilidades serão declaradas em uma tabela Lua semelhante a:

```lua
habilidades = {
    bola_de_fogo = {
        nome = "Bola de Fogo",
        custo = 20,
        dano = 30,
        efeito = "queimadura",
        duracao = 3
    },

    cura = {
        nome = "Cura",
        custo = 15,
        cura = 25
    },

    golpe_venenoso = {
        nome = "Golpe Venenoso",
        custo = 10,
        dano = 15,
        efeito = "veneno",
        duracao = 4
    }
}
```

### Retorno em caso de sucesso

```lua
{
    sucesso = true,
    tipo = "habilidade",
    custo = 20,
    dano = 30,
    cura = 0,
    efeito = "queimadura",
    duracao = 3,
    mensagem = "Herói usou Bola de Fogo."
}
```

### Retorno em caso de falha

```lua
{
    sucesso = false,
    mensagem = "Energia insuficiente."
}
```

### Campos do retorno de habilidade

| Campo | Obrigatório | Tipo Lua | Descrição |
|---|---:|---|---|
| `sucesso` | sim | `boolean` | Informa se a habilidade pode ser executada |
| `mensagem` | sim | `string` | Mensagem de resultado ou erro |
| `tipo` | quando houver sucesso | `string` | Deve ser `habilidade` |
| `custo` | quando houver sucesso | `number` | Energia consumida |
| `dano` | não | `number` | Dano inicial da habilidade e, quando `efeito` for `queimadura` ou `veneno`, dano aplicado a cada turno do efeito |
| `cura` | não | `number` | Cura descrita pela habilidade |
| `efeito` | não | `string` ou `nil` | Efeito de status |
| `duracao` | não | número inteiro | Duração do efeito |

Valores ausentes de `dano`, `cura`, `custo` e `duracao` devem ser tratados como zero.

Mesmo quando Lua retornar `sucesso = true`, o C++ deve confirmar se há energia suficiente e se todos os dados são válidos antes de aplicar a habilidade.

---

## 6. Configuração de dificuldade

Os scripts de dificuldade poderão disponibilizar uma tabela global:

```lua
configuracao = {
    multiplicador_vida = 1.0,
    multiplicador_ataque = 1.0,
    chance_critico = 0.10,
    cura_habilitada = true
}
```

### Campos

| Campo | Tipo Lua | Regra |
|---|---|---|
| `multiplicador_vida` | `number` | Deve ser maior que zero |
| `multiplicador_ataque` | `number` | Deve ser maior que zero |
| `chance_critico` | `number` | Deve estar entre `0.0` e `1.0` |
| `cura_habilitada` | `boolean` | Define se o inimigo pode usar cura |

O C++ deve rejeitar valores fora desses limites e aplicar valores-padrão seguros.

`DifficultyLoader` trata cada campo de maneira independente. Campo ausente, com
tipo incorreto, não finito ou fora do intervalo gera aviso e usa o valor-padrão:

```text
multiplicador_vida = 1.0
multiplicador_ataque = 1.0
chance_critico = 0.10
cura_habilitada = true
```

Erro de sintaxe, erro de execução ou ausência da tabela `configuracao` rejeita o
script inteiro e preserva a configuração carregada anteriormente.

---

## 7. Configuração de arena

Cada script de arena deverá disponibilizar uma tabela global:

```lua
arena = {
    nome = "Arena Vulcânica",
    descricao = "O calor fortalece ataques de fogo.",
    modificadores = {
        dano_fogo = 1.25,
        cura = 0.90
    }
}
```

### Campos obrigatórios

| Campo | Tipo Lua | Descrição |
|---|---|---|
| `nome` | `string` | Nome da arena |
| `descricao` | `string` | Explicação curta do ambiente |
| `modificadores` | `table` | Conjunto de modificadores numéricos |

Os modificadores devem ser números não negativos. Campos desconhecidos podem ser ignorados com registro de aviso.

### Modificadores reconhecidos

| Campo Lua | Uso |
|---|---|
| `dano_fogo` | Fator aplicado a dano de fogo |
| `dano_veneno` | Fator aplicado a dano de veneno |
| `cura` | Fator aplicado a valores de cura |

O fator neutro é `1.0`. Quando um campo reconhecido não aparece na tabela,
`ArenaConfig::modifier` devolve esse fator sem exigir que o script o declare.
Nome e descrição vazios, modificadores com tipo incorreto e valores negativos,
`NaN` ou infinitos rejeitam a arena.

---

## 8. Eventos de arena

O sistema ficará limitado aos três hooks abaixo:

```lua
ao_iniciar_batalha(jogador, inimigo)
ao_iniciar_turno(turno, jogador, inimigo)
ao_finalizar_batalha(resultado, jogador, inimigo)
```

Os hooks são opcionais. Se não existirem, o C++ continua a batalha normalmente.

`ArenaManager` executa cada hook com `lua_pcall`, restaura a Lua Stack depois da
chamada e converte apenas retornos validados. Um hook ausente equivale a `nil`;
um hook com tipo incorreto, que lança erro ou retorna dados inválidos é ignorado
com erro controlado, sem encerrar o processo.

### `ao_iniciar_batalha`

Executado uma vez, antes do primeiro turno.

```lua
function ao_iniciar_batalha(jogador, inimigo)
    return nil
end
```

### `ao_iniciar_turno`

Executado no início de cada turno.

```lua
function ao_iniciar_turno(turno, jogador, inimigo)
    if turno % 3 == 0 then
        return {
            alvo = "todos",
            tipo = "dano",
            valor = 5,
            mensagem = "Uma onda de calor atravessa a arena.",
            efeito = nil,
            duracao = 0
        }
    end

    return nil
end
```

### `ao_finalizar_batalha`

Executado uma vez depois que o motor determinar o resultado.

```lua
function ao_finalizar_batalha(resultado, jogador, inimigo)
    return nil
end
```

O parâmetro `resultado` será uma string com um dos valores:

```text
vitoria
 derrota
```

### Retorno dos eventos

Um hook pode retornar:

- `nil`, quando não houver evento;
- uma tabela de evento.

Formato da tabela:

```lua
{
    alvo = "todos",
    tipo = "dano",
    valor = 5,
    mensagem = "Uma onda de calor atravessa a arena.",
    efeito = nil,
    duracao = 0
}
```

### Alvos permitidos

```text
jogador
inimigo
todos
```

### Tipos permitidos para eventos

```text
dano
cura
defesa
nenhum
```

O C++ continua responsável por validar e aplicar todos os efeitos do evento.

---

## 9. Funções C++ acessíveis por Lua

Lua poderá chamar funções registradas pelo C++.

A primeira função prevista é:

```lua
game_log("O Goblin entrou em modo de desespero.")
```

### Contrato de `game_log`

- recebe exatamente uma `string`;
- não altera o estado da batalha;
- apenas encaminha a mensagem ao sistema de log do motor;
- argumentos inválidos devem gerar erro controlado ou aviso, sem encerrar abruptamente o programa.

Uma segunda função concreta poderá ser registrada para atender à demonstração de bindings, desde que seja documentada antes de seu uso pelos scripts.

### `obter_turno_atual`

```lua
local turno = obter_turno_atual()
```

- não recebe argumentos;
- retorna um inteiro não negativo;
- o valor pertence ao estado Lua atual e é atualizado pelo C++ antes de
  `ao_iniciar_turno`;
- chamada com argumentos gera erro Lua controlado.

A Arena Vulcânica usa essa função para confirmar o turno que dispara a onda de
calor.

---

## 10. Validação obrigatória no C++

Todo retorno de Lua deve ser considerado não confiável até ser validado.

O C++ deve rejeitar ou corrigir de forma segura:

- retorno que não seja uma tabela quando uma tabela for exigida;
- função global inexistente ou com tipo diferente de `function`;
- campos obrigatórios ausentes;
- campos com tipos incorretos;
- números negativos de dano, cura, energia ou duração;
- valores `NaN` ou infinitos;
- duração que não seja inteira;
- alvos desconhecidos;
- tipos de ação desconhecidos;
- efeitos desconhecidos;
- custo maior que a energia disponível;
- cura que ultrapasse a vida máxima;
- dano, cura ou evento aplicado depois do fim da batalha;
- chance de crítico fora do intervalo de `0.0` a `1.0`;
- multiplicadores inválidos;
- erros gerados durante `lua_pcall`.

Quando um campo opcional estiver ausente, o C++ deve usar seu valor-padrão. Quando um campo obrigatório estiver ausente ou inválido, a ação não deve ser aplicada.

---

## 11. Regras para a Lua Stack

Em toda chamada do C++ para Lua, o motor deverá:

1. registrar o topo atual da stack quando necessário;
2. obter a função global;
3. confirmar que o valor obtido é uma função;
4. empilhar os argumentos na ordem definida pelo contrato;
5. executar com `lua_pcall`;
6. verificar e registrar mensagens de erro;
7. validar a quantidade e os tipos dos retornos;
8. converter os dados para estruturas C++;
9. remover os valores temporários;
10. restaurar ou limpar a stack antes de continuar.

Nenhuma chamada pode deixar valores residuais que prejudiquem as chamadas seguintes.

---

## 12. Convenções gerais

- Funções e campos Lua usarão `snake_case`.
- Identificadores de habilidades não terão espaços nem acentos, por exemplo `bola_de_fogo`.
- Textos exibidos ao usuário podem usar português e acentuação normalmente.
- O C++ não deve depender de uma ordem específica dos campos de uma tabela Lua.
- Scripts devem retornar uma única tabela, exceto hooks sem evento, que podem retornar `nil`.
- Alterações futuras neste contrato devem ser discutidas antes de modificar scripts ou interfaces C++ dependentes.

---

## 13. Resumo das interfaces

```lua
-- Inimigos
escolher_acao(inimigo, jogador)

-- Habilidades
usar_habilidade(nome, jogador, inimigo)

-- Arena
ao_iniciar_batalha(jogador, inimigo)
ao_iniciar_turno(turno, jogador, inimigo)
ao_finalizar_batalha(resultado, jogador, inimigo)

-- Binding C++ acessível por Lua
game_log(mensagem)
obter_turno_atual()
```

O guia operacional e os cenários de teste ficam em
[`arenas-e-eventos.md`](arenas-e-eventos.md). Este documento deve ser usado como
referência por todos os integrantes durante a implementação e os testes de
integração.
