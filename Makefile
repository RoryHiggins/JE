CC := gcc
CFLAGS := -std=c99 -Wall -Wextra -pedantic -Werror=vla -Winvalid-pch -fno-exceptions -O0
LFLAGS := -lz -lluajit-5.1 -lcsfml-system -lcsfml-window -lcsfml-graphics

bin/client: Makefile bin src/client/stdafx.h.gch src/client/*.c src/client/*.h
	$(CC) $(CFLAGS) src/client/main.c $(LFLAGS) -o bin/client
src/client/stdafx.h.gch: Makefile src/client/stdafx.h
	$(CC) $(CFLAGS) src/client/stdafx.h
bin:
	mkdir -p bin

run: bin/client
	./bin/client
clean:
	rm -f bin/client src/client/stdafx.h.gch game_dump.sav game_save.sav

.PHONY: run run_headless clean
