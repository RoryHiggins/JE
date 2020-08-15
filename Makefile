CC := gcc
CFLAGS := -std=c99 -Wall -Wextra -pedantic -Werror=vla

CFLAGS_DEVELOPMENT := $(CFLAGS) -Winvalid-pch -O0
LFLAGS_DEVELOPMENT := -lluajit-5.1 -lz -lcsfml-system -lcsfml-window -lcsfml-graphics

CFLAGS_RELEASE := $(CFLAGS) -fno-exceptions -Os -s -ffast-math -flto -fwhole-program
LFLAGS_RELEASE := -mwindows -lmingw32 -static-libstdc++ -static-libgcc -Wl,-Bstatic -lz -lluajit-5.1 -Wl,-Bdynamic -lcsfml-system -lcsfml-window -lcsfml-graphics

client: Makefile bin stdafx.h.gch src/client/main.c src/client/*.h
	$(CC) $(CFLAGS_DEVELOPMENT) src/client/main.c $(LFLAGS_DEVELOPMENT) -o client
stdafx.h.gch: Makefile src/client/stdafx.h
	$(CC) $(CFLAGS_DEVELOPMENT) src/client/stdafx.h
bin:
	mkdir -p bin

release:
	rm -rf release
	mkdir release
	luajit -b src/game/main.lua main.lua.obj
	ar rcs main.lua.a main.lua.obj
	$(CC) $(CFLAGS_RELEASE) -fprofile-use src/client/main.c main.lua.a $(LFLAGS_RELEASE) -o release/client
	$(CC) $(CFLAGS_RELEASE) -fprofile-generate src/client/main.c main.lua.a $(LFLAGS_RELEASE) -o client_profiled
	cp -r src/ lib/ data/ bin/* -- release/
run: client
	./client
clean:
	rm -f client client_profiled game_dump.sav game_save.sav stdafx.h.gch main.lua.obj main.lua.a main.gcda
	rm -rf release

.PHONY: release run clean
