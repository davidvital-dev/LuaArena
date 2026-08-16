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
