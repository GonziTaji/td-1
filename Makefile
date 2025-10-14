CC = gcc
CFLAGS = -Wall -Iinclude
DEBUGFLAGS = -g -Werror
RAYLIB_FLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

SRC := $(shell find src -name "*.c")
OBJ := $(patsubst src/%.c, build/%.o, $(SRC))
OUT = build/main

all: compile_commands.json $(OUT)

# Enlazar objetos para crear el ejecutable
$(OUT): $(OBJ)
	@mkdir -p $(dir $@)
	@cp -r resources build/
	$(CC) $(OBJ) -o $@ $(RAYLIB_FLAGS)

# Compilar cada .c a .o manteniendo la estructura
build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) ${DEBUGFLAGS} -c $< -o $@

# Generar compile_commands.json con compiledb
compile_commands.json: $(SRC) Makefile
	@echo ">> Generating compile_commands.json with compiledb..."
	@compiledb -n make $(OUT)

clean:
	rm -rf build compile_commands.json
