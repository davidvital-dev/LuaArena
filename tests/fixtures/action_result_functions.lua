-- Funções de teste para LuaEngine::callActionFunction (docs/contracts.md, seção 3).

function acao_valida()
    return {
        tipo = "ataque",
        valor = 15,
        mensagem = "Ataque padrão."
    }
end

function acao_com_opcionais()
    return {
        tipo = "habilidade",
        valor = 30,
        mensagem = "Bola de Fogo.",
        efeito = "queimadura",
        duracao = 3,
        custo = 20
    }
end

function acao_retorno_nao_table()
    return "isso não é uma tabela"
end

function acao_sem_tipo()
    return {
        valor = 10,
        mensagem = "Falta o campo tipo."
    }
end

function acao_valor_invalido()
    return {
        tipo = "ataque",
        valor = "dez",
        mensagem = "Valor não numérico."
    }
end

function acao_mensagem_invalida()
    return {
        tipo = "ataque",
        valor = 10,
        mensagem = 42
    }
end

function acao_duracao_invalida()
    return {
        tipo = "habilidade",
        valor = 10,
        mensagem = "Duração inválida.",
        duracao = "tres"
    }
end
