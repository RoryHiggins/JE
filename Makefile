# RELEASE, PROFILED, DEVELOPMENT, DEBUG, TRACE
TARGET := DEVELOPMENT

CC := gcc
CFLAGS := -std=c99 -Wall -Wextra -pedantic -Werror=vla

CFLAGS_RELEASE := $(CFLAGS) -fno-exceptions -Os -s -ffast-math -flto -fwhole-program -mwindows -D NDEBUG
LFLAGS_RELEASE := -static `sdl2-config --static-libs` -lm -lpng -ljpeg -lz -lopengl32 -lglu32 -lglew32 -lluajit-5.1 -static-libstdc++ -static-libgcc

CFLAGS_PROFILED := $(CFLAGS_RELEASE) -pg -fno-inline-functions -fno-omit-frame-pointer
LFLAGS_PROFILED := $(LFLAGS_RELEASE)

CFLAGS_DEVELOPMENT := $(CFLAGS) -Winvalid-pch -O0
LFLAGS_DEVELOPMENT := `sdl2-config --libs` -lm -lpng -ljpeg -lz -lopengl32 -lglu32 -lglew32 -lluajit-5.1

CFLAGS_DEBUG := $(CFLAGS_DEVELOPMENT) -O0 -ggdb3 -fno-inline-functions -fno-omit-frame-pointer -fexceptions -D JE_BUILD_DEBUG
LFLAGS_DEBUG := $(LFLAGS_DEVELOPMENT)

CFLAGS_TRACE := $(CFLAGS_DEBUG) -D JE_BUILD_TRACE
LFLAGS_TRACE := $(LFLAGS_DEBUG)

client_launcher: Makefile stdafx.h.gch client/*.c client/*.h
	$(CC) $(CFLAGS_$(TARGET)) -D JE_BUILD_$(TARGET) client/main.c $(LFLAGS_$(TARGET)) -o client_launcher
stdafx.h.gch: Makefile client/stdafx.h
	$(CC) $(CFLAGS_$(TARGET)) -D JE_BUILD_$(TARGET) client/stdafx.h -o stdafx.h.gch

release:
	rm -rf release
	mkdir release
	mkdir release/src
	$(CC) $(CFLAGS_RELEASE) -D JE_BUILD_RELEASE client/main.c $(LFLAGS_RELEASE) -o release/client_launcher
	cp -r lib/ data/ -- release/
	cp -r src/engine src/game -- release/src
	tar -czvf j25_`date +"%Y_%m_%d_%H_%M_%S"`.tar.gz -- release/*
run: client_launcher
	./client_launcher
profile: gmon.out
	gprof -b client_launcher* gmon.out > profile.txt
clean:
	rm -f client_launcher game_dump.sav game_save.sav stdafx.h.gch gmon.out profile.txt
	rm -rf release

.PHONY: release run profile clean
