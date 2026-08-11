arena = {
    nome = "Floresta Venenosa",
    descricao = "Esporos tóxicos castigam periodicamente quem atravessa a floresta.",
    modificadores = {
        dano_veneno = 1.10
    }
}

local INTERVALO_ESPOROS = 2
local DANO_ESPOROS = 3
local DURACAO_VENENO = 2

function ao_iniciar_turno(turno, jogador, inimigo)
    if turno % INTERVALO_ESPOROS ~= 0 then
        return nil
    end

    return {
        alvo = "jogador",
        tipo = "dano",
        valor = DANO_ESPOROS,
        mensagem = jogador.nome .. " respirou os esporos venenosos.",
        efeito = "veneno",
        duracao = DURACAO_VENENO
    }
end
