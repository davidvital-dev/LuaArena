# LuaArena

Jogo de batalha em terminal, escrito em C++, cujo comportamento pode ser estendido por scripts Lua sem recompilar o programa.

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

Retorno comum de uma ação:

```lua
{
    alvo = "inimigo",
    tipo = "dano",
    valor = 10,
    mensagem = "Descrição da ação.",
    efeito = nil,
    duracao = 0
}
```

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
├── presentation/      # material de apresentação
└── tests/            # testes
```

## Dependências

- `g++` com suporte a C++17;
- `pkg-config`;
- biblioteca de desenvolvimento da Lua 5.4 (ex: `sudo apt install liblua5.4-dev`).

## Build e execução

```bash
make check-deps  # valida se as dependências estão instaladas
make build       # compila o motor e gera build/lua-arena
make run         # compila (se necessário) e executa o jogo
make clean       # remove os artefatos de build
make rebuild     # clean + build
```

O binário final fica em `build/lua-arena`.

## Equipe

| Nome | GitHub | Responsabilidade |
|---|---|---|
| David | [`davidvital-dev`](https://github.com/davidvital-dev) | Integração central C++/Lua (`LuaEngine`, contratos) |
| Carlos | [`carlossan25c`](https://github.com/carlossan25c) | Motor do jogo (`Character`, `Game`, dano, cura, turnos) |
| Levi | [`lfariazzz`](https://github.com/lfariazzz) | Inimigos, comportamento e dificuldade |
| Jetro | [`jetrokepler`](https://github.com/jetrokepler) | Habilidades e fundamentação teórica |
| Henrique | [`HenriqueCoimbra12`](https://github.com/HenriqueCoimbra12) | Build, bindings, erros e testes |
| Ângelo | [`Angelo-Gabriel-Dev`](https://github.com/Angelo-Gabriel-Dev) | Arenas, eventos e validação de extensões |

## Restrições

Este projeto não implementa interface gráfica, multiplayer, mapa, banco de dados, login, inventário complexo, servidor ou campanha.

## Status

Projeto em desenvolvimento inicial: estrutura de diretórios definida, implementação do motor e das extensões Lua em andamento.

## Licença

Distribuído sob a licença MIT. Veja [LICENSE](LICENSE) para mais detalhes.
