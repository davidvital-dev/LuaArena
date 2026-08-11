configuracao = setmetatable({}, {
    __index = function()
        error("metamethod de dificuldade nao deve ser executado")
    end
})
