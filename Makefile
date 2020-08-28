CC := gcc
CFLAGS := -std=c89 -Wall -Wextra -pedantic -Werror=vla

CFLAGS_DEVELOPMENT := $(CFLAGS) -Winvalid-pch -O0
LFLAGS_DEVELOPMENT := -lluajit-5.1 -lz -lcsfml-system -lcsfml-window -lcsfml-graphics

CFLAGS_RELEASE := $(CFLAGS) -fno-exceptions -Os -s -ffast-math -flto -fwhole-program
LFLAGS_RELEASE := -mwindows -lmingw32 -static-libstdc++ -static-libgcc -Wl,-Bstatic -lz -lluajit-5.1 -Wl,-Bdynamic -lcsfml-system -lcsfml-window -lcsfml-graphics

client: Makefile precompiled.h.gch src/client/*.c src/client/*.h
	$(CC) $(CFLAGS_DEVELOPMENT) src/client/*.c $(LFLAGS_DEVELOPMENT) -o client
release/client:
	rm -rf release
	mkdir release
	$(CC) $(CFLAGS_RELEASE) src/client/*.c $(LFLAGS_RELEASE) -o release/client
	cp -r src/ lib/ data/ bin/* -- release/
	tar -czvf j25_`date +"%Y_%m_%d_%H_%M_%S"`.tar.gz -- release/*
precompiled.h.gch: Makefile src/client/precompiled.h
	$(CC) $(CFLAGS_DEVELOPMENT) src/client/precompiled.h -o precompiled.h.gch

run: client
	./client
run_release: release/client
	release/client
clean:
	rm -f client game_dump.sav game_save.sav precompiled.h.gch
	rm -rf release

.PHONY: release/client run run_release clean
