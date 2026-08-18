#!/usr/bin/env bash

set -u

output="$(timeout 5 ./build/lua-arena scripts/enemies/goblin_basic.lua </dev/null)"
status=$?

if [[ $status -eq 124 ]]; then
    echo "[FALHA] EOF deixou o loop principal em execução"
    exit 1
fi

if [[ $status -ne 0 ]]; then
    echo "[FALHA] jogo retornou $status ao receber EOF"
    exit 1
fi

if ! grep -q "Entrada encerrada\." <<<"$output"; then
    echo "[FALHA] jogo não informou o encerramento da entrada"
    exit 1
fi

if grep -q "venceu!" <<<"$output"; then
    echo "[FALHA] EOF continuou a batalha até produzir vencedor"
    exit 1
fi

echo "[OK] EOF encerra o loop principal"

regen_output="$(printf '4\n' | timeout 5 ./build/lua-arena scripts/enemies/goblin_basic.lua)"
regen_status=$?

if [[ $regen_status -eq 124 ]]; then
    echo "[FALHA] teste de regeneração deixou o loop principal em execução"
    exit 1
fi

if [[ $regen_status -ne 0 ]]; then
    echo "[FALHA] jogo retornou $regen_status no teste de regeneração"
    exit 1
fi

if ! grep -q "recupera 5 de energia\." <<<"$regen_output"; then
    echo "[FALHA] energia não foi recuperada no início do turno seguinte"
    exit 1
fi

if ! grep -q "25/30 energia" <<<"$regen_output"; then
    echo "[FALHA] saldo de energia após regeneração não ficou em 25/30"
    exit 1
fi

echo "[OK] jogador recupera 5 de energia por turno até o máximo"
