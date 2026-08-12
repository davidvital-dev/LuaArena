function acao_valida_contrato()
    return {
        tipo = "ataque",
        valor = 12,
        mensagem = "Ataque validado pelo contrato.",
        efeito = "veneno",
        duracao = 2,
        custo = 0
    }
end

function acao_sem_mensagem()
    return {
        tipo = "ataque",
        valor = 10
    }
end

function acao_tipo_invalido()
    return {
        tipo = "teleporte",
        valor = 0,
        mensagem = "Tipo não permitido."
    }
end

function acao_valor_negativo()
    return {
        tipo = "ataque",
        valor = -1,
        mensagem = "Valor negativo."
    }
end

function acao_valor_infinito()
    return {
        tipo = "ataque",
        valor = math.huge,
        mensagem = "Valor infinito."
    }
end

function acao_efeito_invalido()
    return {
        tipo = "habilidade",
        valor = 5,
        mensagem = "Efeito inválido.",
        efeito = "congelamento"
    }
end

function acao_duracao_negativa()
    return {
        tipo = "habilidade",
        valor = 5,
        mensagem = "Duração inválida.",
        efeito = "veneno",
        duracao = -1
    }
end

function acao_custo_negativo()
    return {
        tipo = "habilidade",
        valor = 5,
        mensagem = "Custo inválido.",
        custo = -10
    }
end

function acao_metatable_hostil()
    return setmetatable({
        tipo = "ataque",
        valor = 5
    }, {
        __index = function()
            error("metamethod não deve ser executado durante leitura do contrato")
        end
    })
end
