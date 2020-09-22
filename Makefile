# RELEASE, PROFILED, DEVELOPMENT, DEBUG, TRACE
TARGET := DEVELOPMENT
GAME := games/game
RELEASE_TARGET := RELEASE

CC := gcc
CFLAGS := -std=c99 -Wall -Wextra -pedantic -Werror=vla

CFLAGS_RELEASE := $(CFLAGS) -fno-exceptions -Os -s -ffast-math -flto -fwhole-program -mwindows -D NDEBUG
SDL_FLAGS_RELEASE := `sdl2-config --static-libs --cflags`
LFLAGS_RELEASE := -static $(SDL_FLAGS_RELEASE) -lm -lpng -lz -lopengl32 -lglu32 -lglew32 -lluajit-5.1 -static-libstdc++ -static-libgcc

CFLAGS_PROFILED := $(CFLAGS_RELEASE) -pg -fno-inline-functions -fno-omit-frame-pointer
LFLAGS_PROFILED := $(LFLAGS_RELEASE)

CFLAGS_DEVELOPMENT := $(CFLAGS) -Winvalid-pch -O0
SDL_FLAGS_DEVELOPMENT := `sdl2-config --libs --cflags | sed -e 's/-mwindows//g'` # exclude -mwindows to 
LFLAGS_DEVELOPMENT := $(SDL_FLAGS_DEVELOPMENT) -lm -lpng -lz -lopengl32 -lglu32 -lglew32 -lluajit-5.1

CFLAGS_DEBUG := $(CFLAGS_DEVELOPMENT) -O0 -ggdb3 -fno-inline-functions -fno-omit-frame-pointer -fexceptions -D JE_BUILD_DEBUG
LFLAGS_DEBUG := $(LFLAGS_DEVELOPMENT)

CFLAGS_TRACE := $(CFLAGS_DEBUG) -D JE_BUILD_TRACE
LFLAGS_TRACE := $(LFLAGS_DEBUG)

engine_client: Makefile stdafx.h.gch client/*.c client/*.h
	$(CC) $(CFLAGS_$(TARGET)) -D JE_BUILD_$(TARGET) client/main.c $(LFLAGS_$(TARGET)) -o engine_client
stdafx.h.gch: Makefile client/stdafx.h
	$(CC) $(CFLAGS_$(TARGET)) -D JE_BUILD_$(TARGET) client/stdafx.h -o stdafx.h.gch

release:
	rm -rf release
	mkdir release
	$(CC) $(CFLAGS_$(RELEASE_TARGET)) -D JE_BUILD_$(RELEASE_TARGET) client/main.c $(LFLAGS_$(RELEASE_TARGET)) -o release/engine_client

	mkdir release/client
	cp -r client/data -- release/client

	cp -r engine -- release

	mkdir release/games
	cp -r $(GAME) -- release/games

	tar -czvf j25_`date +"%Y_%m_%d_%H_%M_%S"`.tar.gz -- release/*
run: engine_client
	./engine_client $(GAME)
run_debugger: engine_client
	gdb --args ./engine_client $(GAME)
profile: gmon.out
	gprof -b engine_client* gmon.out > profile.txt
docs:
	scripts/build_docs_html.sh
clean:
	rm -f engine_client game_dump.sav game_save.sav stdafx.h.gch gmon.out profile.txt
	rm -rf release

.PHONY: release run run_debugger profile docs clean
