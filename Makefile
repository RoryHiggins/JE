# RELEASE, PROFILED, DEVELOPMENT, DEBUG, TRACE
TARGET := DEVELOPMENT
GAME := games/game

CC := gcc
LUA := luajit
PYTHON := python3

CFLAGS_COMMON := -std=c99 -Wall -Wextra -pedantic -Werror=vla -DJE_DEFAULT_GAME_DIR=\"$(GAME)\"
LFLAGS_COMMON := -lm -lpng -lz -lGL -lGLU -lGLEW -lluajit-5.1
ifeq ($(OS),Windows_NT)
	LFLAGS_COMMON := -lm -lpng -lz -lopengl32 -lglu32 -lglew32 -lluajit-5.1
endif

SDL_FLAGS_RELEASE := `sdl2-config --static-libs --cflags`
CFLAGS_RELEASE := $(CFLAGS_COMMON) -fno-exceptions -Os -s -ffast-math -flto -fwhole-program -mwindows -D NDEBUG
LFLAGS_RELEASE := -static $(SDL_FLAGS_RELEASE) $(LFLAGS_COMMON) -static-libstdc++ -static-libgcc

CFLAGS_PROFILED := $(CFLAGS_RELEASE) -pg -fno-inline-functions -fno-omit-frame-pointer
LFLAGS_PROFILED := $(LFLAGS_RELEASE)

SDL_FLAGS_DEVELOPMENT := `sdl2-config --libs --cflags | sed -e 's/-mwindows//g'` # exclude -mwindows
CFLAGS_DEVELOPMENT := $(CFLAGS_COMMON) -Winvalid-pch -O0
LFLAGS_DEVELOPMENT := $(SDL_FLAGS_DEVELOPMENT) $(LFLAGS_COMMON)

CFLAGS_DEBUG := $(CFLAGS_DEVELOPMENT) -O0 -g3 -ggdb3 -fno-inline-functions -fno-omit-frame-pointer -fexceptions
LFLAGS_DEBUG := $(LFLAGS_DEVELOPMENT)
ifneq ($(OS),Windows_NT)
	CFLAGS_DEBUG := $(CFLAGS_DEBUG) -fsanitize=address,leak,undefined -fsanitize-recover=address -fsanitize-address-use-after-scope -fstack-protector
endif

CFLAGS_TRACE := $(CFLAGS_DEBUG)
LFLAGS_TRACE := $(LFLAGS_DEBUG)

engine_client: Makefile private_dependencies.h.gch client/*.c client/*.h
	$(CC) $(CFLAGS_$(TARGET)) -D JE_BUILD_$(TARGET) client/main.c $(LFLAGS_$(TARGET)) -o engine_client
private_dependencies.h.gch: Makefile client/private_dependencies.h
	$(CC) $(CFLAGS_$(TARGET)) -D JE_BUILD_$(TARGET) client/private_dependencies.h -o private_dependencies.h.gch

run: engine_client
	./engine_client -game $(GAME)
run_headless:
	$(LUA) $(GAME)/main.lua
run_debugger: engine_client
	gdb --args ./engine_client -game $(GAME)
profile: gmon.out
	gprof -b engine_client* gmon.out > profile.txt
docs:
	$(PYTHON) -m pip install -r scripts/requirements.txt
	$(PYTHON) scripts/build_docs.py --src-dir engine/docs/src --build-dir engine/docs/build
release:
	rm -rf release
	mkdir release
	$(CC) $(CFLAGS_RELEASE) -D JE_BUILD_RELEASE client/main.c $(LFLAGS_RELEASE) -o release/engine_client

	mkdir release/client
	cp -r client/data -- release/client

	cp -r engine -- release

	mkdir release/games
	cp -r $(GAME) -- release/games

	tar -czvf j25_`date +"%Y_%m_%d_%H_%M_%S"`.tar.gz -- release/*
clean:
	rm -f engine_client game_dump.sav game_save.sav private_dependencies.h.gch gmon.out profile.txt
	rm -rf release engine/docs/build

.PHONY: run run_debugger run_headless profile docs release clean
