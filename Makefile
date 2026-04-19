default: build run

clean:
	rm ./calendar

build:
	gcc -Wall -Wextra -g -o calendar main.c `pkg-config --cflags --libs sdl2 SDL2_ttf fontconfig`

run:
	./calendar
