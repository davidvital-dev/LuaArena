# Conceitos de Lua

## Características

Lua é normalmente descrita como uma linguagem de múltiplos paradigmas, oferecendo um pequeno conjunto de características gerais que podem ser estendidas para encaixar diferentes tipos de problemas, em vez de fornecer uma especificação mais complexa e rígida para combinar com um único paradigma [LUA-SPE].

Este documento cobre sintaxe, tipos e os mecanismos internos que dão a Lua sua flexibilidade (metatables, coletor de lixo, retornos múltiplos). Os mecanismos ligados à incorporação de Lua em uma aplicação hospedeira — embedding, runtime, máquina virtual, bytecode e stack da API C — já estão descritos em [`contextualizacao.md`](contextualizacao.md), aplicados diretamente ao Lua Arena, e não são repetidos aqui.

## Mecanismos de funcionamento

### Sintaxe e semântica

Esta seção descreve o vocabulário, aspectos sintáticos e semânticos de Lua. Em outras palavras, descreve quais itens de vocabulário são válidos, como combiná-los e o que significam.

#### Convenções léxicas

Em Lua, um nome (também chamado de identificador) pode ser qualquer string de letras, números e sublinhados que não comece com um número. Os identificadores são usados para nomear variáveis e campos de tabela.

As seguintes palavras-chave são reservadas e não podem ser usadas como nomes:

```
and   break  do    else   elseif
end   false  for   function  if
in    local  nil   not    or
repeat return then true   until  while
```

Lua é uma linguagem que diferencia maiúsculas de minúsculas: `and` é uma palavra reservada, mas `And` e `AND` são dois nomes válidos diferentes. Por convenção, nomes que começam com sublinhado e letras maiúsculas (como `_VERSION`) são reservados para variáveis globais internas usadas por Lua.

As seguintes cadeias denotam outros itens léxicos:

```
+   -   *   /   %   ^   #
==  ~=  <=  >=  <   >   =
(   )   {   }   [   ]
;   :   ,   .   ..  ...
```

#### Sintaxe e exemplos

O programa "Olá Mundo" pode ser escrito da seguinte forma:

```lua
print("Olá, Mundo!")
```

**Algoritmo de Trabb Pardo-Knuth**

```lua
local function f(t)
  return math.sqrt(math.abs(t)) + 5 * t ^ 3
end

local a = {}
for i = 1, 11 do
  a[i] = io.read("*number")
end

for i = #a, 1, -1 do
  local y = f(a[i])
  print(i - 1, y > 400 and "TOO LARGE" or y)
end
```

**Funções**

A função fatorial recursiva:

```lua
function fact(n)
   if n == 0 then
      return 1
   else
      return n * fact(n - 1)
   end
end
```

O cálculo dos n primeiros números perfeitos:

```lua
function perfeitos(n)
   cont = 0
   x = 0
   print("Os números perfeitos são:")
   repeat
      x = x + 1
      soma = 0
      for i = 1, (x - 1) do
         if math.mod(x, i) == 0 then soma = soma + i end
      end
      if soma == x then
         print(x)
         cont = cont + 1
      end
   until cont == n
   print("Pressione qualquer tecla para finalizar...")
end
```

O tratamento das funções como variáveis de primeira classe é mostrado no exemplo a seguir, onde o comportamento da função `print` é modificado:

```lua
do
   local oldprint = print -- Grava a variável "print" em "oldprint"
   print = function(s)    -- Redefine a função "print"
      if s == "foo" then
         oldprint("bar")
      else
         oldprint(s)
      end
   end
end
```

Qualquer chamada da função `print` agora será executada através da nova função, e graças ao escopo léxico de Lua, a função `print` antiga só será acessível pela nova.

Lua também suporta funções closure, como demonstrado abaixo:

```lua
function makeaddfunc(x) -- Retorna uma nova função que adiciona x ao argumento
   return function(y)
      return x + y
   end
end
plustwo = makeaddfunc(2)
print(plustwo(5)) -- Prints 7
```

Um novo closure é criado para a variável `x` cada vez que a função `makeaddfunc` é chamada, de modo que a função anônima retornada sempre irá acessar seu próprio parâmetro `x`. O closure é gerenciado pelo coletor de lixo (garbage collector) da linguagem, tal como qualquer outro objeto.

As funções em Lua são apenas uma categoria de variáveis. Elas podem ser definidas, também, como se fossem uma:

```lua
local foo = function(a, b, c)
    print('variable "foo"')
end
```

Ou podem estar dentro de tabelas:

```lua
local myTable = {'item 1', function() print('table item 2') end, 3}
```

#### Estruturas de repetição

O Lua fornece 4 tipos diferentes de loops: `while`, `repeat` (similar ao `do while` de outras linguagens), `for` e o loop genérico. Suas respectivas sintaxes são demonstradas abaixo:

```lua
while condição do
    -- Comandos
end

repeat
    -- Comandos
until condição

for i = início, fim, passo do -- O passo pode ser positivo ou negativo
    -- Comandos
    -- Exemplo: print(i)
end

for key, value in next, some_table do
    -- Comandos
end
```

Em relação ao laço genérico, ele atua sobre cada par de dados presente na tabela. Esse tipo de laço é semelhante ao que encontramos em Python.

A seguir, um exemplo simples de utilização:

```lua
local info = { John = 10, Alex = 32 } -- Declaração de uma tabela com pares chave/valor

for key, value in next, info do
    -- "key" e "value" serão as variáveis utilizadas
    -- elas armazenarão a chave da tabela (o nome)
    -- e o valor dessa chave (a idade)
    print("Name: ", key)
    print("Age: ", value)
end

-- Saída:
-- Name: John
-- Age: 10
-- Name: Alex
-- Age: 32
```

### Tipos e estruturas de dados suportados

- Lua é uma linguagem que suporta apenas um pequeno número de estruturas, tais como dados atômicos, valores booleanos, números (dupla precisão em ponto flutuante por padrão), e strings.
- As estruturas de dados comuns, tais como matrizes, conjuntos, tabelas, listas, e registros, podem ser representadas por meio de Lua.

### Metatables e metamétodos

Lua não foi construída com suporte nativo à programação orientada a objetos: não há classes, herança ou operadores sobrecarregáveis por padrão. Esse comportamento é obtido por meio de **metatables**, tabelas associadas a outra tabela que definem como ela reage a operações como somar (`+`), comparar (`==`), indexar um campo inexistente ou ser chamada como função [LUA-MANUAL].

O metamétodo `__index`, por exemplo, permite que uma tabela "herde" campos de outra quando um campo não é encontrado diretamente nela — é assim que a maioria das bibliotecas Lua simula classes e herança. Essa é a base que projetos de jogos usam para modelar entidades (personagens, itens, habilidades) como se fossem objetos, mesmo sem suporte nativo a OOP.

### Retornos múltiplos e varargs

Diferente da maioria das linguagens, uma função Lua pode retornar mais de um valor em uma única chamada, e pode receber um número variável de argumentos (`...`). Isso reduz a necessidade de estruturas auxiliares só para agrupar retornos, e é usado com frequência para funções que devolvem ao mesmo tempo um resultado e um indicador de sucesso ou erro [LUA-MANUAL].

### Coletor de lixo

Lua gerencia memória automaticamente por meio de um **coletor de lixo (garbage collector)** incremental: tabelas, funções (incluindo closures) e outros objetos deixam de existir apenas quando não há mais nenhuma referência alcançável a eles [LUA-MANUAL]. Isso evita que o script precise liberar memória manualmente, mas também significa que o momento exato da liberação não é previsível — algo relevante quando o script interage com recursos controlados pelo hospedeiro, como discutido em [`contextualizacao.md`](contextualizacao.md).

## Aplicações reais

Lua concentra suas aplicações em papéis de extensão e configuração, e não como linguagem principal de um sistema [LUA-SPE; LUA-EVOLUTION]:

- **Jogos** — Lua se popularizou em engines e jogos comerciais por ser fácil de embutir e rápida o bastante para lógica de gameplay. A LucasArts usou Lua para roteirizar eventos de *Grim Fandango* (1997), um dos primeiros usos conhecidos em jogos comerciais; a Blizzard usa Lua para toda a interface (addons) de *World of Warcraft*; engines como Corona SDK e Love2D adotam Lua como linguagem de script principal para jogos 2D.
- **Automação e configuração de aplicações** — ferramentas como o Nmap (scripts de varredura de rede) e o Wireshark usam Lua para permitir que o próprio usuário estenda o comportamento do programa sem recompilá-lo, o mesmo papel que Lua cumpre no Lua Arena.
- **Edição de conteúdo e documentação** — desde 2013, a Wikimedia Foundation usa Lua (via extensão Scribunto) para escrever predefinições (templates) da Wikipédia, substituindo uma sintaxe de templates mais limitada por uma linguagem de programação real.
- **Outras aplicações** — controle de robôs, processamento de texto e scripting de aplicações de mercado (ex.: plugins do Adobe Photoshop Lightroom) também usam Lua pelos mesmos motivos: leveza, portabilidade e facilidade de embutir.

No Lua Arena, esse papel de "linguagem de extensão embutida" é o mesmo: o motor em C++ mantém o estado autoritativo da batalha, e Lua descreve habilidades e decisões dentro da fronteira definida em [`contextualizacao.md`](contextualizacao.md) e [`contracts.md`](contracts.md) — o mesmo padrão de uso encontrado nos exemplos acima.
