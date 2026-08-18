arena = {
    nome = "Arena de Metamethod",
    descricao = "Garante leitura raw dos contratos Lua.",
    modificadores = {}
}

setmetatable(_G, {
    __index = function()
        error("metamethod global nao deve ser executado")
    end
})

function ao_iniciar_turno()
    return setmetatable({
        alvo = "inimigo",
        tipo = "dano",
        valor = 1,
        mensagem = "Evento lido sem executar __index."
    }, {
        __index = function()
            error("metamethod de evento nao deve ser executado")
        end
    })
end
