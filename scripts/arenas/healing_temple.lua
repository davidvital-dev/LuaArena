arena = {
    nome = "Templo de Cura",
    descricao = "A energia do templo restaura a vida sem exceder seu limite máximo.",
    modificadores = {
        cura = 1.20
    }
}

local INTERVALO_CURA = 3
local VALOR_CURA = 8

function ao_iniciar_turno(turno, jogador, inimigo)
    if turno % INTERVALO_CURA ~= 0 then
        return nil
    end

    return {
        alvo = "todos",
        tipo = "cura",
        valor = VALOR_CURA,
        mensagem = "A energia do templo restaura os combatentes.",
        efeito = nil,
        duracao = 0
    }
end
