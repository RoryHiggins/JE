CC := gcc
CFLAGS := -std=c89 -Wall -Wextra -pedantic -Werror=vla

CFLAGS_RELEASE := $(CFLAGS) -fno-exceptions -Os -s -ffast-math -flto -fwhole-program -mwindows -D NDEBUG
LFLAGS_RELEASE := -static `sdl2-config --static-libs` -lm -lpng -ljpeg -lz -lopengl32 -lglu32 -lglew32 -lluajit-5.1 -static-libstdc++ -static-libgcc

CFLAGS_PROFILE := $(CFLAGS_RELEASE) -Os -g3 -pg -fno-inline-functions
LFLAGS_PROFILE := $(LFLAGS_RELEASE)

CFLAGS_DEVELOPMENT := $(CFLAGS) -Winvalid-pch -O0
LFLAGS_DEVELOPMENT := -lm -lpng -ljpeg -lz `sdl2-config --static-libs` -lopengl32 -lglu32 -lglew32 -lluajit-5.1

CFLAGS_DEBUG := $(CFLAGS_DEVELOPMENT) -Og -g3 -fno-inline-functions
LFLAGS_DEBUG := $(LFLAGS_DEVELOPMENT)

DEVELOPMENT_MODE := DEBUG
RELEASE_MODE := RELEASE

bin/client: Makefile stdafx.h.gch src/client/*.c src/client/*.h
	$(CC) $(CFLAGS_$(DEVELOPMENT_MODE)) -D JE_BUILD_$(DEVELOPMENT_MODE) src/client/main.c $(LFLAGS_$(DEVELOPMENT_MODE)) -o bin/client
release/client:
	rm -rf release
	mkdir release
	$(CC) $(CFLAGS_$(RELEASE_MODE)) -D JE_BUILD_$(DEVELOPMENT_MODE) src/client/main.c $(LFLAGS_$(RELEASE_MODE)) -o release/client
	cp -r src/ lib/ data/ -- release/
	tar -czvf j25_`date +"%Y_%m_%d_%H_%M_%S"`.tar.gz -- release/*
stdafx.h.gch: Makefile src/client/stdafx.h
	$(CC) $(CFLAGS_$(DEVELOPMENT_MODE)) -D JE_BUILD_$(DEVELOPMENT_MODE) src/client/stdafx.h -o stdafx.h.gch

run: bin/client
	./bin/client
run_release: release/client
	release/client
profile: gmon.out
	gprof -b bin/client* gmon.out > profile.txt
clean:
	rm -f bin/client game_dump.sav game_save.sav stdafx.h.gch gmon.out profile.txt
	rm -rf release

.PHONY: release/client run run_release clean
