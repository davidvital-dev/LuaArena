arena = {
    nome = "Arena de retornos inválidos",
    descricao = "Exercita cada barreira de validação do C++.",
    modificadores = {}
}

function ao_iniciar_turno(turno, jogador, inimigo)
    if turno == 1 then
        return {
            alvo = "plateia",
            tipo = "dano",
            valor = 1,
            mensagem = "Alvo inválido."
        }
    elseif turno == 2 then
        return {
            alvo = "todos",
            tipo = "teleporte",
            valor = 1,
            mensagem = "Tipo inválido."
        }
    elseif turno == 3 then
        return {
            alvo = "todos",
            tipo = "dano",
            valor = "muito",
            mensagem = "Valor inválido."
        }
    elseif turno == 4 then
        return {
            alvo = "todos",
            tipo = "dano",
            valor = 1,
            mensagem = "Efeito inválido.",
            efeito = "sono",
            duracao = 2
        }
    elseif turno == 5 then
        return {
            alvo = "todos",
            tipo = "dano",
            valor = 1,
            mensagem = "Duração inválida.",
            efeito = "veneno",
            duracao = 1.5
        }
    elseif turno == 6 then
        return {
            alvo = "todos",
            tipo = "dano",
            valor = 1
        }
    elseif turno == 7 then
        error("falha intencional do hook")
    elseif turno == 8 then
        return "não é uma tabela"
    elseif turno == 9 then
        return {
            alvo = "inimigo",
            tipo = "dano",
            valor = 2,
            mensagem = "A stack continuou íntegra.",
            efeito = nil,
            duracao = 0
        }
    end
    return nil
end

ao_finalizar_batalha = "não é uma função"
