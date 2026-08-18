# LuaArena

Jogo de batalha em terminal, escrito em C++, cujo comportamento pode ser estendido por scripts Lua sem recompilar o programa.

## Contexto acadêmico

- **Instituição:** Universidade Federal do Cariri (UFCA);
- **Disciplina:** Paradigmas da Programação;
- **Professor:** Rafael Will Macedo de Araujo.

## Regra arquitetural

```text
Lua decide ou descreve.
C++ valida e aplica.
```

O motor em C++ é responsável por validar e aplicar tudo o que os scripts Lua descrevem. Os scripts Lua nunca acessam ou alteram o estado do jogo diretamente.

## O que é estendido via Lua

- comportamento dos inimigos;
- habilidades;
- dificuldade;
- arenas;
- eventos de ciclo de vida;
- mensagens e parâmetros modificáveis.

## Contratos (hooks Lua)

```lua
escolher_acao(inimigo, jogador)
usar_habilidade(nome, jogador, inimigo)
ao_iniciar_batalha(jogador, inimigo)
ao_iniciar_turno(turno, jogador, inimigo)
ao_finalizar_batalha(resultado, jogador, inimigo)
```

Retorno da escolha de ação de um inimigo:

```lua
{
    tipo = "ataque",
    valor = 10,
    mensagem = "Descrição da ação.",
    efeito = nil,
    duracao = 0,
    custo = 0
}
```

Os formatos completos de ações, habilidades, arenas, dificuldade e eventos
estão em [`docs/contracts.md`](docs/contracts.md).

## Arenas obrigatórias

- Neutra;
- Vulcânica;
- Floresta Venenosa;
- Templo de Cura.

## Estrutura do projeto

```text
LuaArena/
├── src/              # código-fonte C++ (motor, bindings, integração com Lua)
├── scripts/
│   ├── abilities/    # habilidades em Lua
│   ├── difficulty/   # modos de dificuldade em Lua
│   ├── arenas/       # arenas em Lua
│   └── enemies/      # comportamento dos inimigos em Lua
├── docs/             # documentação de arquitetura e uso
├── diagrams/         # diagramas do projeto
├── presentation/     # material de apresentação
└── tests/            # testes
```

## Dependências

- `g++` com suporte a C++17;
- `pkg-config`;
- biblioteca de desenvolvimento da Lua 5.4 (ex: `sudo apt install liblua5.4-dev`).

Em Debian, Ubuntu ou WSL/Ubuntu:

```bash
sudo apt update
sudo apt install build-essential make pkg-config liblua5.4-dev
```

### Windows

C++ e Lua são multiplataforma, portanto a arquitetura do LuaArena não é limitada ao Linux. Entretanto, a versão entregue do projeto foi configurada e validada em ambiente Linux.

No Windows, a forma mais simples de executar o projeto atualmente é através do **WSL (Windows Subsystem for Linux)**, utilizando uma distribuição como Ubuntu. Dentro do WSL, os mesmos comandos de instalação, build, testes e execução usados no Linux podem ser utilizados.

O `Makefile` atual depende de ferramentas e comandos de ambiente POSIX, como `rm`, `mkdir`, `test`, `make` e `pkg-config`. Por esse motivo, ele **não foi preparado nem validado para execução direta pelo `cmd.exe` ou PowerShell**.

A execução nativa no Windows também é tecnicamente possível, por exemplo com **MSYS2/MinGW** ou **Visual Studio/MSVC**, mas exige adaptar o processo de build e a vinculação da biblioteca Lua. Essa adaptação não altera a arquitetura C++ ↔ Lua nem os scripts do jogo; trata-se principalmente de uma diferença na cadeia de compilação.

Em resumo:

- **Linux:** ambiente utilizado e validado pelo projeto;
- **Windows + WSL:** caminho recomendado para utilizar o projeto no Windows com o build atual;
- **Windows nativo:** viável, mas requer adaptação do sistema de build e não faz parte da versão entregue.

## Build e execução

```bash
make check-deps  # valida se as dependências estão instaladas
make clean       # remove os artefatos de build
make build       # compila o motor e gera build/lua-arena
make run         # compila (se necessário) e executa o jogo
make test        # compila e executa todas as suítes automatizadas
make rebuild     # clean + build
```

O binário final fica em `build/lua-arena`.

Para executar escolhendo inimigo, arena e dificuldade:

```bash
./build/lua-arena scripts/enemies/goblin_basic.lua \
  --arena scripts/arenas/volcanic.lua \
  --difficulty scripts/difficulty/normal.lua
```

Arena e dificuldade são opcionais. Sem essas flags, o jogo usa
`scripts/arenas/neutral.lua` e `scripts/difficulty/normal.lua`.

Para comprovar a troca de comportamento sem recompilar, execute o mesmo
binário com os dois scripts:

```bash
./build/lua-arena scripts/enemies/goblin_basic.lua
./build/lua-arena scripts/enemies/goblin_aggressive.lua
```

## Arenas e dificuldade

As arenas ficam em `scripts/arenas/` e podem ser trocadas em tempo de execução,
sem recompilar o C++. A configuração de dificuldade segue o mesmo princípio em
`scripts/difficulty/`. O motor carrega os scripts, valida todos os campos e só
então disponibiliza eventos para aplicação no estado da batalha.

Consulte [`docs/arenas-e-eventos.md`](docs/arenas-e-eventos.md) para o contrato,
o ciclo de vida, exemplos de integração e o comportamento de cada arena.

## Testes

```bash
make check-deps
make clean
make build
make test
```

O alvo `make test` executa as suítes de arenas e dificuldade, motor do jogo,
integração C++ ↔ Lua e o teste do fluxo de EOF do executável. O roteiro completo,
incluindo os cenários manuais, está em [`docs/testes.md`](docs/testes.md).

## Motor do jogo

O núcleo C++ mantém personagens, vida, energia, turnos, vitória, derrota e
efeitos temporários. Consulte [`docs/motor-do-jogo.md`](docs/motor-do-jogo.md)
para a API, os invariantes e o fluxo recomendado de integração com Lua.

## Documentação

- [`docs/contracts.md`](docs/contracts.md): contratos C++ ↔ Lua;
- [`docs/build-bindings-e-execucao.md`](docs/build-bindings-e-execucao.md): build, bindings e execução;
- [`docs/arenas-e-eventos.md`](docs/arenas-e-eventos.md): arenas e eventos;
- [`docs/motor-do-jogo.md`](docs/motor-do-jogo.md): regras do motor;
- [`docs/testes.md`](docs/testes.md): roteiro de testes;
- [`docs/contextualizacao.md`](docs/contextualizacao.md): contexto e características da Lua;
- [`docs/comparacao-lua-python.md`](docs/comparacao-lua-python.md): comparação Lua × Python;
- [`docs/referencias.md`](docs/referencias.md): referências bibliográficas.

## Equipe

| Nome | GitHub | Responsabilidade |
|---|---|---|
| David Josué Vital Santos | [`davidvital-dev`](https://github.com/davidvital-dev) | Integração central C++/Lua (`LuaEngine`, contratos) |
| Carlos Santos | [`carlossan25c`](https://github.com/carlossan25c) | Motor do jogo (`Character`, `Game`, dano, cura, turnos) |
| Levi Farias | [`lfariazzz`](https://github.com/lfariazzz) | Inimigos, comportamento e dificuldade |
| Jetro Kepler Gomes | [`jetrokepler`](https://github.com/jetrokepler) | Habilidades e fundamentação teórica |
| Henrique Coimbra | [`HenriqueCoimbra12`](https://github.com/HenriqueCoimbra12) | Build, bindings, erros e testes |
| Ângelo Gabriel | [`Angelo-Gabriel-Dev`](https://github.com/Angelo-Gabriel-Dev) | Arenas, eventos e validação de extensões |

## Restrições

Este projeto não implementa interface gráfica, multiplayer, mapa, banco de dados, login, inventário complexo, servidor ou campanha.

## Status

Protótipo jogável integrado: inimigos, habilidades, dificuldade e arenas são
carregados por scripts Lua, enquanto o motor C++ valida e aplica as ações. O
projeto possui build automatizado, testes de integração e fluxo de batalha até
vitória ou derrota.

## Licença

Distribuído sob a licença MIT. Veja [LICENSE](LICENSE) para mais detalhes.
