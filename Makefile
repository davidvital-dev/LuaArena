# =============================================================
# Lua Arena — Makefile
# Compila o motor em C++ e linka com a biblioteca Lua 5.4
# =============================================================

CXX       := g++
CXXFLAGS  := -std=c++17 -Wall -Wextra -O2 -Isrc

# Detecta as flags de compilação/link da Lua via pkg-config.
# Se o pkg-config não encontrar "lua5.4", tenta "lua-5.4" e depois "lua".
LUA_PKG   := $(shell pkg-config --exists lua5.4 && echo lua5.4 || \
                (pkg-config --exists lua-5.4 && echo lua-5.4 || \
                (pkg-config --exists lua && echo lua)))

ifeq ($(LUA_PKG),)
$(error Nao foi possivel encontrar a biblioteca Lua via pkg-config. \
Instale o pacote de desenvolvimento da Lua 5.4 (ex: sudo apt install liblua5.4-dev) \
e rode 'make check-deps' para validar)
endif

LUA_CFLAGS := $(shell pkg-config --cflags $(LUA_PKG))
LUA_LIBS   := $(shell pkg-config --libs $(LUA_PKG))

CXXFLAGS   += $(LUA_CFLAGS)
LDLIBS     := $(LUA_LIBS)

# Diretórios
SRC_DIR   := src
BUILD_DIR := build

# Descobre todos os .cpp em src/ automaticamente
SRCS      := $(wildcard $(SRC_DIR)/*.cpp)
OBJS      := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

TARGET    := $(BUILD_DIR)/lua-arena

# =============================================================
# Alvos principais
# =============================================================

.PHONY: all build run clean check-deps rebuild

all: build

build: $(TARGET)

$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CXX) $(OBJS) -o $@ $(LDLIBS)
	@echo ""
	@echo "==> Build concluído: $(TARGET)"

# Regra genérica: compila cada .cpp em .o dentro de build/
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: build
	./$(TARGET) scripts/enemies/goblin_basic.lua

clean:
	rm -rf $(BUILD_DIR)

rebuild: clean build

# Confirma que as dependências necessárias estão instaladas
check-deps:
	@echo "Verificando compilador..."
	@command -v $(CXX) >/dev/null 2>&1 && echo "  [ok] $(CXX) encontrado" || \
		(echo "  [FALTA] $(CXX) nao encontrado"; exit 1)
	@echo "Verificando pkg-config..."
	@command -v pkg-config >/dev/null 2>&1 && echo "  [ok] pkg-config encontrado" || \
		(echo "  [FALTA] pkg-config nao encontrado"; exit 1)
	@echo "Verificando biblioteca Lua..."
	@test -n "$(LUA_PKG)" && echo "  [ok] Lua encontrada como pacote '$(LUA_PKG)'" || \
		(echo "  [FALTA] nenhum pacote lua5.4/lua-5.4/lua encontrado via pkg-config"; exit 1)
	@echo ""
	@echo "Todas as dependências estão OK."
