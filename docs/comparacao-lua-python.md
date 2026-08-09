# Lua e Python como linguagens embarcadas

## 1. Escopo da comparação

Lua e Python podem ser incorporadas a aplicações C e C++. Ambas oferecem APIs nativas, execução dinâmica e mecanismos para expor funções do hospedeiro aos scripts [LUA-MANUAL; PY-EMBED]. A escolha depende dos requisitos: integração disponível, distribuição, bibliotecas necessárias, modelo de segurança, experiência da equipe e função que os scripts exercerão.

Este documento separa **fatos documentados**, aplicáveis às runtimes consideradas, de **conclusões do Lua Arena**, que dependem do escopo acadêmico do projeto. Não apresenta uma linguagem como universalmente superior.

## 2. Comparação factual

| Critério | Lua 5.4 | CPython 3 | Leitura prática |
|---|---|---|---|
| Integração C/C++ | Lua é implementada como biblioteca C e sua API troca valores por uma stack virtual [LUA-MANUAL]. | A API Python/C permite incorporar o interpretador e manipula objetos Python por ponteiros e funções da API [PY-EMBED; PY-CAPI]. | As duas são viáveis; seus modelos de interoperabilidade e tratamento de recursos são diferentes. |
| Tamanho e runtime | A distribuição oficial contém um núcleo, bibliotecas separadas, interpretador e compilador; o hospedeiro pode abrir apenas bibliotecas selecionadas [LUA-SOURCE; LUA-MANUAL]. | A distribuição embarcável inclui runtime e biblioteca-padrão; extensões e pacotes podem acrescentar dependências nativas e arquivos [PY-WINDOWS]. | Lua tende a exigir menos componentes para scripting restrito; a necessidade real deve ser medida no produto e na plataforma alvo. |
| Inicialização | O hospedeiro cria um `lua_State`, escolhe bibliotecas, carrega chunks e executa chamadas protegidas [LUA-MANUAL]. | O hospedeiro pode configurar e inicializar CPython com `PyConfig` e `Py_InitializeFromConfig`; a configuração isolada controla ambiente e caminhos [PY-CONFIG]. | Python oferece configuração detalhada; Lua expõe um ciclo de incorporação pequeno e direto. |
| Distribuição | Lua é distribuída em fonte ISO C e pode ser compilada como parte ou dependência da aplicação [LUA-README]. | CPython precisa ser distribuído com sua runtime e os módulos necessários; no Windows há um pacote embarcável mínimo e quase isolado [PY-WINDOWS]. | O empacotamento de Python pode envolver mais artefatos, sobretudo quando o script usa pacotes externos. |
| API e dados | Tipos, argumentos e retornos passam pela stack; tabelas são o mecanismo central de estruturação [LUA-MANUAL; LUA-SPE]. | A API opera com `PyObject*`, contagem de referências, exceções e módulos; parte da API possui ABI estável [PY-CAPI; PY-STABLE-ABI]. | Lua favorece contratos pequenos baseados em tabelas; Python oferece um modelo de objetos e módulos mais amplo. |
| Ecossistema | Lua concentra-se em scripting, configuração e extensão, com ecossistema menor e especializado. | Python possui biblioteca-padrão extensa e amplo ecossistema de terceiros para automação, dados, ciência e outras áreas. | A vantagem de Python cresce quando o domínio depende dessas bibliotecas; ela é menos relevante para regras simples de batalha. |
| Segurança | Bibliotecas como `io`, `os`, `package` e `debug` são módulos separados e podem deixar de ser abertas pelo hospedeiro [LUA-MANUAL]. | A configuração isolada reduz influência do ambiente, mas os próprios audit hooks não constituem uma sandbox; módulos perigosos também precisam ser controlados [PY-CONFIG; PY-AUDIT]. | Nenhuma das duas deve executar código hostil sem isolamento adicional. A superfície exposta deve ser mínima. |
| Casos de uso | Configuração, descrição de dados, regras de jogos e extensões pequenas são usos coerentes com o projeto original da linguagem [LUA-SPE; LUA-EVOLUTION]. | Automação rica, integração com pacotes Python, análise de dados e aplicações que já dependem de CPython favorecem Python [PY-EMBED]. | O domínio e as dependências importam mais que uma classificação geral de desempenho. |

## 3. Integração e modelo de API

### 3.1 Lua

Lua foi concebida para extensão de aplicações. A biblioteca fornece operações para criar um estado, carregar código, empilhar argumentos, executar uma função e extrair resultados. As tabelas permitem transportar registros sem exigir que o script conheça o layout das classes C++ [LUA-SPE].

Esse modelo é adequado a uma fronteira estreita, mas exige disciplina manual: índices corretos, checagem de tipos, chamadas protegidas e restauração da stack. A simplicidade da API não elimina a necessidade de validação.

### 3.2 Python

CPython também pode ser incorporado e a documentação oficial demonstra como o programa C/C++ inicializa o interpretador, importa módulos, obtém funções e chama objetos Python [PY-EMBED]. A API expõe o modelo de objetos de Python e exige o tratamento correto de referências e exceções.

A Limited API e a Stable ABI reduzem parte do acoplamento binário entre versões quando somente seu subconjunto é usado [PY-STABLE-ABI]. Isso é uma opção de compatibilidade, não uma garantia automática para toda extensão ou pacote de terceiros.

## 4. Runtime, inicialização e distribuição

Comparações de “leveza” devem evitar números universais. Tamanho em disco, memória, tempo de inicialização e desempenho dependem da versão, compilador, plataforma, bibliotecas abertas, módulos importados e carga de trabalho. Sem um benchmark reproduzível do Lua Arena, este documento faz apenas uma conclusão estrutural: o conjunto oficial de componentes Lua necessário às regras do jogo é menor e mais específico que uma distribuição CPython com sua biblioteca-padrão e eventuais pacotes.

Lua pode ser compilada a partir da distribuição em C e ligada ao executável [LUA-README]. CPython oferece mecanismos formais de configuração e, no Windows, uma distribuição própria para incorporação que inclui a runtime e parte da biblioteca-padrão em arquivos separados [PY-CONFIG; PY-WINDOWS]. Em ambos os casos, o projeto precisa controlar versões e incluir as dependências no processo de build e entrega.

## 5. Segurança e fronteira de confiança

Uma runtime incorporada não é automaticamente uma sandbox. O hospedeiro precisa decidir quais funções, bibliotecas, módulos, caminhos e recursos ficam acessíveis.

Em Lua, é possível abrir individualmente as bibliotecas necessárias. Para regras de batalha, não há motivo para expor por padrão acesso a arquivos, sistema operacional, carregamento de módulos nativos ou depuração. Também são necessárias validação dos retornos, limites de domínio e tratamento de erros.

Em CPython, a configuração isolada evita várias influências do ambiente do usuário, mas não transforma código Python arbitrário em código seguro. A documentação alerta que audit hooks não bastam como sandbox e cita módulos capazes de acesso a memória como risco [PY-AUDIT]. Código não confiável exigiria isolamento de processo ou mecanismo equivalente, independentemente da linguagem escolhida.

## 6. Conclusão específica do Lua Arena

Para o Lua Arena, Lua é a escolha mais coerente porque:

- o objetivo é demonstrar integração C++/script com uma API pequena e explícita;
- as regras atuais usam números, strings, booleanos e tabelas, sem depender de um grande ecossistema de pacotes;
- o contrato “Lua descreve; C++ valida e aplica” corresponde diretamente ao modelo de extensão da linguagem;
- habilidades e configurações podem mudar sem recompilar o motor, desde que permaneçam dentro do contrato;
- a stack torna visível, para fins didáticos, a passagem de argumentos, retornos e erros entre as linguagens;
- o hospedeiro pode disponibilizar somente as bibliotecas e bindings necessários.

Python seria uma alternativa justificável se o projeto precisasse reutilizar bibliotecas específicas de seu ecossistema, integrar uma base de código Python existente, oferecer automação mais abrangente ou adotar ferramentas já centradas em CPython. Esses cenários não fazem parte do escopo atual.

Portanto, a decisão não é que Lua seja sempre mais rápida, segura ou adequada que Python. A conclusão é mais restrita: para um motor C++ acadêmico, com scripts curtos de batalha e uma fronteira de dados pequena, Lua satisfaz os requisitos com menos elementos conceituais e de distribuição.

As referências identificadas entre colchetes estão descritas e mapeadas em [`referencias.md`](referencias.md).
