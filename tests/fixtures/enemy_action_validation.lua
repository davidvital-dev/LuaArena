tipo_acao = "ataque"
efeito_acao = nil
duracao_acao = 0
custo_acao = 0
valor_acao = 5

function escolher_acao(inimigo, jogador)
    return {
        tipo = tipo_acao,
        valor = valor_acao,
        mensagem = inimigo.nome .. " descreveu uma ação de teste.",
        efeito = efeito_acao,
        duracao = duracao_acao,
        custo = custo_acao
    }
end
