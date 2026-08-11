-- Script de inimigo inválido para testes: compila normalmente, mas não
-- define escolher_acao, exigida pelo contrato (docs/contracts.md, seção 4).

nome_padrao = "Goblin sem comportamento"

function preparar_inimigo()
    nome_padrao = "Goblin preparado"
end
