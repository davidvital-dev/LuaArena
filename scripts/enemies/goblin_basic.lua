-- Comportamento de referência: sempre ataca com o valor-base de ataque,
-- sem considerar o estado do jogador.
function escolher_acao(inimigo, jogador)
    return {
        tipo = "ataque",
        valor = inimigo.ataque,
        mensagem = inimigo.nome .. " ataca de forma direta.",
        efeito = nil,
        duracao = 0,
        custo = 0
    }
end
