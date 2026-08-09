local function criar_habilidade(nome, custo, dano, cura, efeito, duracao)
    return {
        nome = nome,
        custo = custo,
        dano = dano,
        cura = cura,
        efeito = efeito,
        duracao = duracao
    }
end
habilidades = {}

local function registrar_habilidade(
    identificador,
    nome,
    custo,
    dano,
    cura,
    efeito,
    duracao
)
    habilidades[identificador] = criar_habilidade(
        nome,
        custo,
        dano,
        cura,
        efeito,
        duracao
    )
end

local function falha(mensagem)
    return {
        sucesso = false,
        mensagem = mensagem
    }
end

function usar_habilidade(nome, jogador, inimigo)
    if type(nome) ~= "string" or nome == "" then
        return falha("Habilidade inválida.")
    end

    if type(jogador) ~= "table" or type(inimigo) ~= "table" then
        return falha("Dados dos personagens inválidos.")
    end

    local habilidade = habilidades[nome]
    if habilidade == nil then
        return falha("Habilidade desconhecida: " .. nome .. ".")
    end

    local nome_jogador = jogador.nome
    if type(nome_jogador) ~= "string" or nome_jogador == "" then
        nome_jogador = "Jogador"
    end

    return {
        sucesso = true,
        tipo = "habilidade",
        custo = habilidade.custo,
        dano = habilidade.dano,
        cura = habilidade.cura,
        efeito = habilidade.efeito,
        duracao = habilidade.duracao,
        mensagem = nome_jogador .. " usou " .. habilidade.nome .. "."
    }
end
