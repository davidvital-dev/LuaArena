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
