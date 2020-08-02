CC := gcc
CFLAGS := -std=c99 -Wall -Wextra -pedantic -Werror=vla -Winvalid-pch -fno-exceptions -O0
LFLAGS := -lz -lluajit-5.1 -lcsfml-system -lcsfml-window -lcsfml-graphics
CFLAGS_HEADLESS := $(CFLAGS) -D JE_HEADLESS
LFLAGS_HEADLESS := -lz -lluajit-5.1

client: Makefile client.h.gch client.c
	$(CC) $(CFLAGS) client.c $(LFLAGS) -o client
client_headless: Makefile client.h client.c
	$(CC) $(CFLAGS_HEADLESS) client.c $(LFLAGS_HEADLESS) -o client_headless

client.h.gch: Makefile client.h
	$(CC) $(CFLAGS) client.h

run: client
	.\client
run_headless: client_headless
	.\client_headless
clean:
	rm -f client client_headless client.h.gch

.PHONY: run run_headless clean
