CC := gcc
CFLAGS := -std=c99 -Wall -Wextra -pedantic -Werror=vla -Winvalid-pch -fno-exceptions -O0
CFLAGS_HEADLESS := $(CFLAGS) -D JE_HEADLESS
LFLAGS := -lcsfml-system -lcsfml-window -lcsfml-graphics -lluajit-5.1
LFLAGS_HEADLESS := -lluajit-5.1

client: client.h.gch client.c
	$(CC) $(CFLAGS) client.c $(LFLAGS) -o client
client_headless: client.c
	$(CC) $(CFLAGS_HEADLESS) client.c $(LFLAGS_HEADLESS) -o client_headless

client.h.gch: client.h
	$(CC) $(CFLAGS) client.h

run: client
	.\client
run_headless: client_headless
	.\client_headless
clean:
	rm -f client client_headless client.h.gch

.PHONY: run run_headless clean
