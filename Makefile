CFLAGS = -Wall -Wextra -Wpedantic -std=c99 -g

default: build run

clean:
	rm ./calendar

build:
	gcc -o calendar main.c `pkg-config --cflags --libs sdl2 SDL2_ttf fontconfig` $(CFLAGS) 

run:
	./calendar
