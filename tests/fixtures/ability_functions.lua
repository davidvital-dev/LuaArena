function usar_habilidade(nome, jogador, inimigo)
    if nome == "falha" then
        return {
            sucesso = false,
            mensagem = "A habilidade foi recusada pelo script."
        }
    end

    if nome == "erro" then
        error("falha intencional da habilidade")
    end

    if nome == "retorno_invalido" then
        return "não é uma tabela"
    end

    if nome == "sem_tipo" then
        return {
            sucesso = true,
            mensagem = "Resultado incompleto.",
            custo = 0
        }
    end

    if nome == "numero_invalido" then
        return {
            sucesso = true,
            tipo = "habilidade",
            mensagem = "Custo inválido.",
            custo = math.huge
        }
    end

    if nome == "duracao_invalida" then
        return {
            sucesso = true,
            tipo = "habilidade",
            mensagem = "Duração inválida.",
            custo = 0,
            efeito = "veneno",
            duracao = 1.5
        }
    end

    if nome == "efeito_invalido" then
        return {
            sucesso = true,
            tipo = "habilidade",
            mensagem = "Efeito inválido.",
            custo = 0,
            efeito = "desconhecido",
            duracao = 2
        }
    end

    if nome == "energia_insuficiente_cpp" then
        return {
            sucesso = true,
            tipo = "habilidade",
            mensagem = "O script descreveu uma habilidade cara.",
            custo = jogador.energia + 1,
            dano = 10
        }
    end

    if nome == "metatable" then
        return setmetatable({
            sucesso = true,
            tipo = "habilidade",
            mensagem = "Leitura raw válida.",
            custo = 0
        }, {
            __index = function()
                error("metamétodo não deve ser executado")
            end
        })
    end

    return {
        sucesso = true,
        tipo = "habilidade",
        mensagem = "Habilidade de teste.",
        custo = 0
    }
end
