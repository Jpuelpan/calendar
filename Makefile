default: build run

clean:
	rm ./calendar

build:
	gcc -Wall -Wextra -g -o calendar main.c `pkg-config --cflags --libs sdl2`

run:
	./calendar
