arena = setmetatable({}, {
    __index = function()
        error("metamethod de arena nao deve ser executado")
    end
})
