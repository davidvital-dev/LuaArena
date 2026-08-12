# Motor do jogo

Este documento descreve o núcleo C++ do Lua Arena: personagens, recursos,
turnos, menu de ações, resultado da batalha e efeitos temporários.

## Princípio arquitetural

```text
Lua decide ou descreve.
C++ valida e aplica.
```

Os scripts escolhem ações e descrevem habilidades, inimigos, dificuldades e
eventos. O motor mantém o estado real da batalha. Alterar uma tabela Lua não
altera diretamente vida, energia ou efeitos de um personagem.

Os arquivos centrais são:

| Arquivo | Responsabilidade |
|---|---|
| `ActionResult.hpp` | representação C++ de uma ação descrita por Lua |
| `StatusEffect.hpp` | estado de um efeito temporário |
| `Character.hpp/.cpp` | atributos, dano, cura, energia e efeitos |
| `Game.hpp/.cpp` | personagens, turnos e resultado da batalha |
| `ActionMenu.hpp/.cpp` | apresentação e leitura das ações do jogador |

## ActionResult

`ActionResult` é o formato comum usado para transportar uma ação validada até
o motor:

```cpp
struct ActionResult {
    std::string type;
    double value = 0.0;
    std::string message;
    std::string effect;
    int duration = 0;
    double energyCost = 0.0;
};
```

Os nomes C++ correspondem aos campos Lua `tipo`, `valor`, `mensagem`,
`efeito`, `duracao` e `custo`. A conversão e a validação dos campos pertencem à
camada de integração. O motor só deve aplicar um `ActionResult` depois dessa
validação.

## StatusEffect

Um efeito temporário armazena:

```cpp
struct StatusEffect {
    std::string name;
    int remainingTurns = 0;
    double valuePerTurn = 0.0;
};
```

O catálogo atual reconhece `queimadura` e `veneno`. Duração e valor por turno
devem ser positivos e finitos antes que o efeito seja armazenado.

## Character

O construtor recebe nome, vida máxima, ataque, defesa e energia máxima. Vida e
energia atuais começam com seus valores máximos:

```cpp
Character mage{"Mago", 100.0, 20.0, 5.0, 50.0};
```

A origem desses atributos deve fornecer valores válidos e não negativos. Os
getters expõem o estado para consulta, sem permitir alteração direta.

### Dano

```cpp
double removedHealth = target.takeDamage(20.0);
```

`takeDamage`:

- rejeita zero, valores negativos, `NaN` e infinito;
- reduz o valor recebido pela defesa do alvo;
- nunca deixa a vida abaixo de zero;
- não modifica personagens já derrotados;
- retorna a vida efetivamente removida.

Por exemplo, um alvo com defesa 5 recebe 15 de dano efetivo quando o valor
informado é 20.

### Cura

```cpp
double recoveredHealth = mage.heal(25.0);
```

`heal` rejeita valores inválidos, limita a vida à vida máxima e retorna a cura
efetiva. Vida cheia não é alterada. O método não revive personagens derrotados.

### Energia

```cpp
if (mage.hasEnoughEnergy(action.energyCost)) {
    mage.spendEnergy(action.energyCost);
}

mage.restoreEnergy(10.0);
```

`spendEnergy` só aceita um custo finito, não negativo e menor ou igual ao saldo.
Uma falha não altera o personagem. `restoreEnergy` limita a recuperação à
energia máxima e retorna o valor efetivamente recuperado.

Mesmo quando Lua declara que uma habilidade teve sucesso, o C++ deve chamar
`hasEnoughEnergy` antes de consumir o custo.

### Queimadura e veneno

```cpp
target.applyBurning(3, 5.0);
target.applyPoison(4, 3.0);

double burningDamage = target.processBurning();
double poisonDamage = target.processPoison();
```

Os dois efeitos:

- causam dano direto, sem redução pela defesa;
- reduzem a duração a cada processamento;
- são removidos quando a duração chega a zero;
- podem permanecer ativos simultaneamente;
- não são aplicados a personagens derrotados;
- rejeitam duração e dano por turno inválidos.

Reaplicar o mesmo efeito atualiza sua duração e seu valor por turno, em vez de
criar uma segunda instância. `isBurning` e `isPoisoned` consultam o estado atual.

O loop de integração decide em qual ponto do turno chamar os métodos de
processamento. Cada método representa exatamente um avanço daquele efeito.

## Game

`Game` é proprietário do jogador e do inimigo:

```cpp
Game game{
    Character{"Mago", 100.0, 20.0, 5.0, 50.0},
    Character{"Goblin", 60.0, 10.0, 3.0, 0.0}
};
```

Há versões mutáveis e constantes de `getPlayer`, `getEnemy`,
`getCurrentCharacter` e `getOpponent`. O código que recebe uma referência
mutável faz parte da fronteira confiável do motor e deve verificar
`isBattleOver()` antes de aplicar uma ação.

### Ciclo de turnos

A batalha começa na rodada 1 com o jogador. A sequência é:

```text
rodada 1: jogador -> inimigo
rodada 2: jogador -> inimigo
...
```

`advanceTurn` alterna o participante atual. A passagem do jogador para o
inimigo mantém o número da rodada; a passagem do inimigo para o jogador inicia
a rodada seguinte.

```cpp
int turn = game.getTurnNumber();
if (game.isPlayerTurn()) {
    // coletar e aplicar a ação do jogador
}

game.advanceTurn();
```

Essa numeração é a mesma enviada para `ao_iniciar_turno` e armazenada pelo
binding `obter_turno_atual`. `advanceTurn` falha depois do fim da batalha ou se
o contador atingir o maior `int` representável.

### Vitória e derrota

`getBattleOutcome` devolve:

| Valor | Condição |
|---|---|
| `InProgress` | jogador e inimigo possuem vida |
| `Victory` | inimigo derrotado |
| `Defeat` | jogador derrotado |

Os atalhos `isBattleOver`, `hasPlayerWon` e `hasPlayerLost` evitam comparações
repetidas. Se ambos estiverem derrotados, derrota tem precedência. A camada de
integração converte `BattleOutcome::Victory` ou `BattleOutcome::Defeat` para o
`BattleResult` esperado por `ArenaManager::onBattleEnd`.

## Menu de ações

`ActionMenu` sempre oferece ataque básico. As habilidades são recebidas como
dados externos para que o C++ não incorpore identificadores ou regras de Lua:

```cpp
ActionMenu menu{{
    {"bola_de_fogo", "Bola de Fogo"},
    {"cura", "Cura"},
    {"golpe_venenoso", "Golpe Venenoso"}
}};

std::optional<PlayerActionSelection> selection =
    menu.readSelection(std::cin, std::cout);
```

Entradas não numéricas, opções fora do intervalo ou texto adicional são
rejeitados. A leitura é repetida até uma opção válida; fim do fluxo de entrada
retorna `std::nullopt`. Identificadores vazios, nomes vazios e identificadores
duplicados tornam a configuração do menu inválida.

O menu apenas coleta a decisão. Para `PlayerActionType::Ability`, a integração
envia `abilityIdentifier` para `usar_habilidade`. O C++ valida o retorno, o
custo e o estado da batalha antes de aplicar qualquer alteração.

## Fluxo de integração recomendado

O loop jogável completo ainda depende da integração entre `Game`, `LuaEngine`,
`ArenaManager` e os scripts. A ordem recomendada é:

1. construir `Game` com atributos já validados;
2. iniciar a arena;
3. disparar o hook de início da rodada com `getTurnNumber()`;
4. processar efeitos temporários no personagem correspondente;
5. obter a ação do jogador pelo menu ou a decisão do inimigo por Lua;
6. validar o retorno e confirmar custo de energia;
7. aplicar dano, cura, energia e efeitos pelo `Character`;
8. consultar `isBattleOver()`;
9. avançar o turno quando a batalha continuar;
10. converter o resultado e disparar o hook de encerramento.

Nenhuma ação ou evento deve ser aplicado depois de `isBattleOver()` retornar
`true`.

## Compilação e testes

O motor requer C++17. No ambiente completo do projeto:

```bash
make check-deps
make build
make test
```

Os contratos C++/Lua completos estão em [`contracts.md`](contracts.md), e o
ciclo das arenas está em [`arenas-e-eventos.md`](arenas-e-eventos.md).

## Limites do escopo

O motor não implementa IA de inimigos, regras internas de habilidades,
dificuldade ou comportamento de arenas. Também não inclui interface gráfica,
mapa, inventário, multiplayer ou persistência. Essas restrições mantêm o foco
na demonstração de Lua como linguagem de extensão para uma aplicação C++.
