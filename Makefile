all: main

SRC=src/main.cpp
main: $(SRC)
	mkdir -p bin
	g++ $(SRC) -F/Library/Frameworks -framework SDL2 -std=c++11 -o bin/snake_astar