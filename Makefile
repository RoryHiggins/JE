CC := gcc
CFLAGS := -std=c89 -Wall -Wextra -pedantic -Werror=vla

CFLAGS_DEVELOPMENT := $(CFLAGS) -Winvalid-pch -O0
LFLAGS_DEVELOPMENT := -lm -lpng -ljpeg -lz `sdl2-config --static-libs` -lopengl32 -lglu32 -lglew32 -lluajit-5.1

CFLAGS_RELEASE := $(CFLAGS) -fno-exceptions -Os -s -ffast-math -flto -fwhole-program -mwindows
LFLAGS_RELEASE := -static `sdl2-config --static-libs` -lm -lpng -ljpeg -lz -lopengl32 -lglu32 -lglew32 -lluajit-5.1 -static-libstdc++ -static-libgcc

bin/client: Makefile core.h.gch src/client/*.c src/client/*.h
	$(CC) $(CFLAGS_DEVELOPMENT) src/client/main.c $(LFLAGS_DEVELOPMENT) -o bin/client
release/client:
	rm -rf release
	mkdir release
	$(CC) $(CFLAGS_RELEASE) src/client/main.c $(LFLAGS_RELEASE) -o release/client
	cp -r src/ lib/ data/ -- release/
	tar -czvf j25_`date +"%Y_%m_%d_%H_%M_%S"`.tar.gz -- release/*
core.h.gch: Makefile src/client/core.h
	$(CC) $(CFLAGS_DEVELOPMENT) src/client/core.h -o core.h.gch

run: bin/client
	./bin/client
run_release: release/client
	release/client
clean:
	rm -f bin/client game_dump.sav game_save.sav core.h.gch
	rm -rf release

.PHONY: release/client run run_release clean
