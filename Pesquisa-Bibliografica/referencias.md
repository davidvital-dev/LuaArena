# Referências bibliográficas

## 1. Critério de organização

As referências estão formatadas de maneira compatível com a ABNT NBR 6023:2018 para documentos consultados on-line. Foram priorizados manuais oficiais, código-fonte mantido pelos projetos e publicações dos autores das linguagens. Todos os links foram verificados em 9 ago. 2026.

Os identificadores entre colchetes são usados nas citações dos documentos [`contextualizacao.md`](contextualizacao.md) e [`comparacao-lua-python.md`](comparacao-lua-python.md).

## 2. Lua

### [LUA-MANUAL]

IERUSALIMSCHY, Roberto; FIGUEIREDO, Luiz Henrique de; CELES, Waldemar. **Lua 5.4 reference manual**. Versão 5.4. [S. l.]: Lua.org, 2025. Disponível em: <https://www.lua.org/manual/5.4/manual.html>. Acesso em: 9 ago. 2026.

Uso no projeto: definição da linguagem; runtime; máquina virtual baseada em registradores; bytecode; API C; stack; estados; chamadas protegidas; bibliotecas-padrão e abertura seletiva de módulos.

### [LUA-README]

LUA.ORG. **Welcome to Lua 5.4**. Versão 5.4. [S. l.]: Lua.org, 2025. Disponível em: <https://www.lua.org/manual/5.4/readme.html>. Acesso em: 9 ago. 2026.

Uso no projeto: composição da distribuição; compilação em ISO C; arquivos necessários para incorporar Lua em aplicações C ou C++.

### [LUA-SOURCE]

LUA.ORG. **Lua 5.4 source code**. Versão 5.4.8. [S. l.]: Lua.org, 2025. Disponível em: <https://www.lua.org/source/5.4/>. Acesso em: 9 ago. 2026.

Uso no projeto: separação entre núcleo, bibliotecas, interpretador e compilador; base factual para discutir componentes do runtime sem atribuir um tamanho universal.

### [LUA-SPE]

IERUSALIMSCHY, Roberto; FIGUEIREDO, Luiz Henrique de; CELES, Waldemar. Lua: an extensible extension language. **Software: Practice & Experience**, [S. l.], v. 26, n. 6, p. 635-652, 1996. Disponível em: <https://www.lua.org/spe.html>. Acesso em: 9 ago. 2026.

Uso no projeto: motivação de Lua como linguagem de extensão; tabelas como mecanismo de descrição de dados; casos de uso e desenho da integração com aplicações hospedeiras.

### [LUA-EVOLUTION]

IERUSALIMSCHY, Roberto; FIGUEIREDO, Luiz Henrique de; CELES, Waldemar. The evolution of Lua. In: ACM SIGPLAN CONFERENCE ON HISTORY OF PROGRAMMING LANGUAGES, 3., 2007, San Diego. **Proceedings [...]**. New York: ACM, 2007. p. 2-1-2-26. Disponível em: <https://www.lua.org/doc/hopl.pdf>. Acesso em: 9 ago. 2026.

Uso no projeto: evolução histórica; objetivos de simplicidade, portabilidade e incorporação; descrição da máquina virtual e das decisões de desenho da linguagem.

## 3. Python

### [PY-EMBED]

PYTHON SOFTWARE FOUNDATION. **Embedding Python in another application**. Python 3.14.6 documentation. [S. l.]: Python Software Foundation, 2026. Disponível em: <https://docs.python.org/3/extending/embedding.html>. Acesso em: 9 ago. 2026.

Uso no projeto: distinção entre estender e incorporar Python; inicialização; importação de módulos; chamada de funções Python a partir de C e C++.

### [PY-CAPI]

PYTHON SOFTWARE FOUNDATION. **Introduction: Python/C API reference manual**. Python 3.14.6 documentation. [S. l.]: Python Software Foundation, 2026. Disponível em: <https://docs.python.org/3/c-api/intro.html>. Acesso em: 9 ago. 2026.

Uso no projeto: modelo de objetos da API C, convenções de erro, referências e elementos necessários à incorporação de CPython.

### [PY-CONFIG]

PYTHON SOFTWARE FOUNDATION. **Python initialization configuration**. Python 3.14.6 documentation. [S. l.]: Python Software Foundation, 2026. Disponível em: <https://docs.python.org/3/c-api/init_config.html>. Acesso em: 9 ago. 2026.

Uso no projeto: `PyConfig`, `Py_InitializeFromConfig`, configuração isolada e controle da inicialização do interpretador incorporado.

### [PY-STABLE-ABI]

PYTHON SOFTWARE FOUNDATION. **C API stability**. Python 3.14.6 documentation. [S. l.]: Python Software Foundation, 2026. Disponível em: <https://docs.python.org/3/c-api/stable.html>. Acesso em: 9 ago. 2026.

Uso no projeto: Limited API e Stable ABI; limites da compatibilidade binária para extensões e aplicações que incorporam CPython.

### [PY-WINDOWS]

PYTHON SOFTWARE FOUNDATION. **Using Python on Windows: the embeddable package**. Python 3.14.6 documentation. [S. l.]: Python Software Foundation, 2026. Disponível em: <https://docs.python.org/3/using/windows.html#the-embeddable-package>. Acesso em: 9 ago. 2026.

Uso no projeto: conteúdo e isolamento da distribuição embarcável; biblioteca-padrão; arquivos da runtime e considerações de distribuição.

### [PY-AUDIT]

PYTHON SOFTWARE FOUNDATION. **`sys` — system-specific parameters and functions: `sys.addaudithook`**. Python 3.14.6 documentation. [S. l.]: Python Software Foundation, 2026. Disponível em: <https://docs.python.org/3/library/sys.html#sys.addaudithook>. Acesso em: 9 ago. 2026.

Uso no projeto: limitação dos audit hooks como mecanismo de sandbox e necessidade de controlar módulos com capacidades privilegiadas.

## 4. Arquitetura de software

### [FOWLER-IOC]

FOWLER, Martin. **Inversion of control**. [S. l.]: MartinFowler.com, 26 jun. 2005. Disponível em: <https://martinfowler.com/bliki/InversionOfControl.html>. Acesso em: 9 ago. 2026.

Uso no projeto: definição geral de inversão de controle e distinção entre o princípio amplo e técnicas específicas, como injeção de dependência.

### [PARNAS-MODULES]

PARNAS, David L. On the criteria to be used in decomposing systems into modules. **Communications of the ACM**, New York, v. 15, n. 12, p. 1053-1058, 1972. DOI: 10.1145/361598.361623. Disponível em: <https://doi.org/10.1145/361598.361623>. Acesso em: 9 ago. 2026.

Uso no projeto: ocultação de decisões de projeto, redução de dependências entre módulos e justificativa para uma fronteira contratual entre motor e scripts.

## 5. Mapa de referências por seção

| Documento e seção | Referências principais |
|---|---|
| `contextualizacao.md` — contexto, hospedeiro e extensão | [LUA-SPE], [LUA-EVOLUTION] |
| `contextualizacao.md` — embedding, runtime, VM, bytecode e stack | [LUA-MANUAL], [LUA-README] |
| `contextualizacao.md` — inversão de controle | [FOWLER-IOC] |
| `contextualizacao.md` — baixo acoplamento e aberto-fechado | [PARNAS-MODULES], [LUA-SPE] |
| `contextualizacao.md` — fronteira de confiança | [LUA-MANUAL], `docs/contracts.md` |
| `comparacao-lua-python.md` — integração e API | [LUA-MANUAL], [PY-EMBED], [PY-CAPI], [PY-STABLE-ABI] |
| `comparacao-lua-python.md` — runtime, inicialização e distribuição | [LUA-README], [LUA-SOURCE], [PY-CONFIG], [PY-WINDOWS] |
| `comparacao-lua-python.md` — segurança | [LUA-MANUAL], [PY-CONFIG], [PY-AUDIT] |
| `comparacao-lua-python.md` — conclusão do Lua Arena | síntese das fontes anteriores e requisitos de `docs/contracts.md` |
| `Conceitos.md` — características, metatables, retornos múltiplos e coletor de lixo | [LUA-MANUAL], [LUA-SPE] |
| `Conceitos.md` — aplicações reais | [LUA-SPE], [LUA-EVOLUTION] |
| `Interoperabilidade-Cpp-Lua.md` — stack, chamadas C++→Lua e Lua→C++ | [LUA-MANUAL] |
