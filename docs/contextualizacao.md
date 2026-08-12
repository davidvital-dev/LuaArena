# Lua como linguagem de extensão no Lua Arena

## 1. Contexto do projeto

O Lua Arena é um jogo acadêmico cujo motor mantém o estado da batalha em C++ e delega decisões configuráveis a scripts Lua. Essa divisão não cria dois motores: o C++ continua responsável por vida, energia, turnos, vitória, derrota e aplicação de efeitos, enquanto Lua descreve ações por meio dos contratos definidos em [`contracts.md`](contracts.md).

Lua foi projetada como uma linguagem leve e incorporável, implementada como biblioteca em C. Seu uso como linguagem de extensão permite que uma aplicação hospedeira ofereça pontos controlados de personalização sem transferir aos scripts a propriedade do processo inteiro [LUA-MANUAL; LUA-SPE]. No Lua Arena, esses pontos são funções como `usar_habilidade`, `escolher_acao` e os eventos de arena.

## 2. Linguagem hospedeira e linguagem de extensão

A **linguagem hospedeira** implementa o programa principal, controla seu ciclo de vida e define quais capacidades serão expostas. Neste projeto, esse papel pertence ao C++.

A **linguagem de extensão** implementa comportamentos carregados pelo programa principal. Ela opera dentro dos limites da interface oferecida pelo hospedeiro. Neste projeto, Lua descreve habilidades, decisões de inimigos, configurações e eventos, mas não é proprietária do estado real do jogo.

Essa relação é diferente de executar dois programas independentes. C++ cria e controla o estado Lua, carrega os scripts, envia argumentos e interpreta os retornos. Lua só conhece os dados que o motor coloca à sua disposição.

## 3. Conceitos fundamentais

### 3.1 Embedding

**Embedding**, ou incorporação, é a inclusão do runtime de uma linguagem dentro de outra aplicação. A aplicação C++ liga a biblioteca Lua, cria um `lua_State`, carrega código e solicita sua execução pela API C. O manual oficial define Lua como uma biblioteca e apresenta a API usada pelo hospedeiro para controlar o estado e a execução [LUA-MANUAL].

No Lua Arena, embedding é a base para executar os arquivos de `scripts/` sem iniciar um processo externo. O motor permanece no comando antes, durante e depois de cada chamada.

### 3.2 Binding

**Binding** é a adaptação entre elementos das duas linguagens. Ele pode expor uma função C++ a Lua, como `game_log`, ou converter dados entre objetos C++ e valores Lua. Um binding inclui decisões sobre nomes, tipos, propriedade dos dados, tratamento de erros e valores-padrão.

No projeto, tabelas Lua representam visões temporárias de jogador e inimigo. Alterar essas tabelas não deve modificar automaticamente os objetos C++; o retorno do script é convertido, validado e só então aplicado pelo motor. Essa regra evita que a adaptação de tipos se transforme em acesso irrestrito ao estado interno.

### 3.3 Runtime

O **runtime** é o conjunto de estruturas e serviços necessários durante a execução da linguagem: estado global, gerenciamento de memória, coletor de lixo, carregador, bibliotecas e mecanismos de erro. Em Lua, cada estado criado pelo hospedeiro reúne o ambiente em que os scripts executam [LUA-MANUAL].

No Lua Arena, a classe responsável pela integração deve cuidar do ciclo de vida desse runtime: inicialização, abertura somente das bibliotecas necessárias, carregamento dos scripts, chamadas protegidas e encerramento.

### 3.4 Máquina virtual e bytecode

Lua compila o código-fonte para instruções internas e as executa em uma **máquina virtual baseada em registradores** [LUA-MANUAL; LUA-EVOLUTION]. Esse formato intermediário é chamado de **bytecode**. Ele não é código de máquina nativo e continua dependendo de um runtime Lua compatível.

Para o projeto, a consequência prática é que os scripts podem ser carregados e executados durante a inicialização ou a batalha sem fazer parte da compilação C++. O projeto deve preferir distribuir os fontes `.lua`: além de serem adequados ao objetivo didático, chunks binários pré-compilados não são um formato portátil entre todas as arquiteturas e versões [LUA-MANUAL].

### 3.5 Stack da API C

A API C usa uma **stack virtual** para trocar valores com o hospedeiro. O C++ coloca a função e seus argumentos na stack; Lua executa; o resultado ou a mensagem de erro volta pela mesma estrutura [LUA-MANUAL]. Índices, quantidade de resultados e tipos precisam ser verificados em cada chamada.

Uma chamada de habilidade segue, conceitualmente, esta sequência:

1. registrar o topo atual da stack;
2. obter e verificar a função global `usar_habilidade`;
3. empilhar `nome`, `jogador` e `inimigo` nessa ordem;
4. executar uma chamada protegida;
5. validar a tabela retornada e convertê-la para tipos C++;
6. restaurar a stack em todos os caminhos, inclusive nos erros.

Valores residuais podem alterar o comportamento de chamadas posteriores. Por isso, a disciplina da stack é parte do contrato de correção, não apenas um detalhe de implementação.

## 4. Consequências arquiteturais

### 4.1 Inversão de controle

Na execução tradicional de uma biblioteca, o programa chama operações e recebe respostas. Em sistemas extensíveis também ocorre uma forma localizada de **inversão de controle**: o hospedeiro define pontos de extensão e, nos momentos previstos, transfere temporariamente o controle ao código fornecido pelo script [FOWLER-IOC].

No Lua Arena, Lua decide o conteúdo de uma ação quando o C++ chama um hook ou uma função contratada. A inversão termina quando a função retorna. O loop da batalha, a validação e a aplicação dos efeitos continuam pertencendo ao motor.

### 4.2 Baixo acoplamento

O acoplamento é reduzido quando motor e scripts dependem de uma interface estável, e não da estrutura interna um do outro. Essa separação segue a ideia de ocultar decisões de projeto que podem mudar independentemente [PARNAS-MODULES]. O Lua Arena usa nomes de funções, parâmetros e tabelas de retorno como fronteira explícita. Assim, o script não precisa conhecer classes C++, e o motor não precisa codificar cada habilidade individualmente.

Esse desacoplamento depende de disciplina: alterar unilateralmente um campo do contrato quebra os consumidores. A centralização em `docs/contracts.md` e os testes de integração são os mecanismos usados pelo grupo para controlar essa evolução.

### 4.3 Princípio aberto-fechado

O princípio aberto-fechado orienta módulos que aceitam extensão de comportamento sem exigir modificação recorrente de seu núcleo. No projeto, um motor que interpreta uma tabela validada pode receber novas habilidades ou ajustes de balanceamento em Lua sem adicionar um novo ramo C++ para cada caso.

Isso não significa que o motor nunca será modificado. Uma mudança no contrato, a introdução de um novo tipo de efeito ou uma nova capacidade privilegiada ainda exige revisão do C++. A aplicação do princípio ocorre dentro da abstração que o contrato atual consegue representar.

### 4.4 Fronteira de confiança

O retorno de um script atravessa uma **fronteira de confiança**. Mesmo que os scripts estejam no próprio repositório, eles podem conter erros, valores fora do domínio, tipos incorretos ou código alterado depois da compilação. Portanto, dados vindos de Lua são entradas não confiáveis até que o motor os valide.

No Lua Arena, o C++ deve confirmar campos obrigatórios, tipos, limites, valores finitos, energia disponível e efeitos permitidos. Também deve usar chamadas protegidas, restringir as bibliotecas expostas e preservar o estado do jogo diante de falhas. O manual permite abrir bibliotecas Lua individualmente, em vez de disponibilizar todas por padrão, o que ajuda a reduzir capacidades desnecessárias [LUA-MANUAL].

## 5. Relação direta entre conceitos e implementação

| Conceito | Manifestação no Lua Arena | Responsabilidade principal |
|---|---|---|
| Hospedeiro | executável e loop da batalha | C++ |
| Linguagem de extensão | habilidades, inimigos, arenas e configuração | Lua |
| Embedding | criação do estado e execução dos scripts no processo | integração C++/Lua |
| Binding | tabelas de personagens, retornos e `game_log` | camada de conversão |
| Runtime/VM | execução e coleta de lixo do estado Lua | biblioteca Lua sob controle do motor |
| Bytecode | representação interna gerada ao carregar um chunk | runtime Lua |
| Stack | passagem de funções, argumentos, retornos e erros | código de integração |
| Inversão de controle | callbacks chamados em pontos definidos pelo motor | contrato dos scripts |
| Baixo acoplamento | dependência em tabelas e funções documentadas | contrato compartilhado |
| Aberto-fechado | novas regras representáveis sem alterar o núcleo | scripts e catálogo |
| Fronteira de confiança | validação antes de aplicar qualquer resultado | C++ |

## 6. Síntese

A arquitetura escolhida usa Lua para variar políticas e conteúdo, e C++ para preservar invariantes. O benefício não vem apenas de trocar recompilação por edição de scripts: vem de estabelecer uma fronteira pequena, verificável e documentada entre decisões dinâmicas e estado autoritativo. Se essa fronteira for respeitada, o projeto ganha extensibilidade sem duplicar o motor nem entregar aos scripts responsabilidades que pertencem ao núcleo.

As referências identificadas entre colchetes estão descritas e mapeadas em [`referencias.md`](referencias.md).
