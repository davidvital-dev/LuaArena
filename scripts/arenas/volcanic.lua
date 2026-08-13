arena = {
    nome = "Arena Vulcânica",
    descricao = "O calor fortalece ataques de fogo e libera ondas abrasadoras.",
    modificadores = {
        dano_fogo = 1.25
    }
}

local INTERVALO_ONDA_DE_CALOR = 3
local DANO_ONDA_DE_CALOR = 5

function ao_iniciar_batalha(jogador, inimigo)
    game_log(
        "A Arena Vulcânica recebeu "
            .. jogador.nome
            .. " e "
            .. inimigo.nome
            .. "."
    )
    return nil
end

function ao_iniciar_turno(turno, jogador, inimigo)
    -- Consulta o binding concreto para demonstrar que o turno pertence ao motor.
    local turno_atual = obter_turno_atual()
    if turno_atual ~= turno or turno_atual % INTERVALO_ONDA_DE_CALOR ~= 0 then
        return nil
    end

    return {
        alvo = "todos",
        tipo = "dano",
        valor = DANO_ONDA_DE_CALOR,
        mensagem = "Uma onda de calor atravessa a arena.",
        efeito = nil,
        duracao = 0
    }
end

function ao_finalizar_batalha(resultado, jogador, inimigo)
    game_log("A batalha vulcânica terminou com " .. resultado .. ".")
    return nil
end
