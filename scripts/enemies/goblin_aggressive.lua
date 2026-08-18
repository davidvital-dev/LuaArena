-- Variação agressiva: intensifica o ataque quando o jogador está com pouca
-- vida, só para provar que o mesmo binário C++ decide de forma diferente
-- dependendo do script carregado.
function escolher_acao(inimigo, jogador)
    local vida_percentual = jogador.vida / jogador.vida_maxima

    if vida_percentual < 0.5 then
        return {
            tipo = "ataque",
            valor = inimigo.ataque * 1.5,
            mensagem = inimigo.nome .. " avança com um golpe de acabamento.",
            efeito = nil,
            duracao = 0,
            custo = 0
        }
    end

    return {
        tipo = "ataque",
        valor = inimigo.ataque,
        mensagem = inimigo.nome .. " ataca sem hesitar.",
        efeito = nil,
        duracao = 0,
        custo = 0
    }
end
