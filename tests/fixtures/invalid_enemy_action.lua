-- Script de inimigo inválido para testes: escolher_acao existe, mas
-- devolve um retorno que viola docs/contracts.md (seção 3) — falta o campo
-- obrigatório "valor". A LuaEngine deve rejeitar sem derrubar o processo.
function escolher_acao(inimigo, jogador)
    return {
        tipo = "ataque",
        mensagem = inimigo.nome .. " tenta agir sem valor definido."
    }
end
