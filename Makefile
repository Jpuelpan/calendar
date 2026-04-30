CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c99 `pkg-config --cflags --libs sdl2 SDL2_ttf fontconfig`

build: main.c
	$(CC) $(CFLAGS) -g -o calendar main.c

clean:
	rm ./calendar

run: build
	./calendar
