dofile("scripts/abilities/abilities.lua")

local function assert_igual(atual, esperado, contexto)
    if atual ~= esperado then
        error(
            contexto
                .. ": esperado "
                .. tostring(esperado)
                .. ", recebido "
                .. tostring(atual)
        )
    end
end

local function novo_jogador(energia)
    return {
        nome = "Herói",
        vida = 80,
        vida_maxima = 100,
        ataque = 20,
        defesa = 5,
        energia = energia,
        energia_maxima = 50
    }
end

local function novo_inimigo()
    return {
        nome = "Goblin",
        vida = 60,
        vida_maxima = 60,
        ataque = 12,
        defesa = 3,
        energia = 0,
        energia_maxima = 0
    }
end

local function testar_sucesso(
    identificador,
    custo,
    dano,
    cura,
    efeito,
    duracao,
    mensagem
)
    local jogador = novo_jogador(50)
    local inimigo = novo_inimigo()
    local vida_jogador = jogador.vida
    local energia_jogador = jogador.energia
    local vida_inimigo = inimigo.vida
    local resultado = usar_habilidade(identificador, jogador, inimigo)

    assert_igual(resultado.sucesso, true, identificador .. " sucesso")
    assert_igual(resultado.tipo, "habilidade", identificador .. " tipo")
    assert_igual(resultado.custo, custo, identificador .. " custo")
    assert_igual(resultado.dano, dano, identificador .. " dano")
    assert_igual(resultado.cura, cura, identificador .. " cura")
    assert_igual(resultado.efeito, efeito, identificador .. " efeito")
    assert_igual(resultado.duracao, duracao, identificador .. " duração")
    assert_igual(resultado.mensagem, mensagem, identificador .. " mensagem")
    assert_igual(jogador.vida, vida_jogador, identificador .. " vida jogador")
    assert_igual(jogador.energia, energia_jogador, identificador .. " energia")
    assert_igual(inimigo.vida, vida_inimigo, identificador .. " vida inimigo")
end

testar_sucesso(
    "bola_de_fogo",
    20,
    30,
    0,
    "queimadura",
    3,
    "Herói usou Bola de Fogo."
)

testar_sucesso("cura", 15, 0, 25, nil, 0, "Herói usou Cura.")

testar_sucesso(
    "golpe_venenoso",
    10,
    15,
    0,
    "veneno",
    4,
    "Herói usou Golpe Venenoso."
)

local energia_exata = usar_habilidade(
    "bola_de_fogo",
    novo_jogador(20),
    novo_inimigo()
)
assert_igual(energia_exata.sucesso, true, "energia exatamente igual ao custo")

local energia_insuficiente = usar_habilidade(
    "bola_de_fogo",
    novo_jogador(19),
    novo_inimigo()
)
assert_igual(energia_insuficiente.sucesso, false, "energia insuficiente")
assert_igual(
    energia_insuficiente.mensagem,
    "Energia insuficiente.",
    "mensagem de energia insuficiente"
)

local entradas_invalidas = {
    usar_habilidade("", novo_jogador(50), novo_inimigo()),
    usar_habilidade("inexistente", novo_jogador(50), novo_inimigo()),
    usar_habilidade("cura", nil, novo_inimigo()),
    usar_habilidade("cura", novo_jogador(50), nil),
    usar_habilidade("cura", novo_jogador("50"), novo_inimigo()),
    usar_habilidade("cura", novo_jogador(-1), novo_inimigo()),
    usar_habilidade("cura", novo_jogador(0 / 0), novo_inimigo()),
    usar_habilidade("cura", novo_jogador(math.huge), novo_inimigo())
}

for indice, resultado in ipairs(entradas_invalidas) do
    assert_igual(resultado.sucesso, false, "entrada inválida " .. indice)
    assert_igual(type(resultado.mensagem), "string", "mensagem inválida " .. indice)
end

assert_igual(habilidades.bola_de_fogo.nome, "Bola de Fogo", "catálogo fogo")
assert_igual(habilidades.cura.nome, "Cura", "catálogo cura")
assert_igual(
    habilidades.golpe_venenoso.nome,
    "Golpe Venenoso",
    "catálogo veneno"
)

print("Todos os testes de habilidades passaram.")
