UNAME := $(shell uname)
all: $(UNAME)

SRC=src/main.cpp

Linux: $(SRC)
	mkdir -p bin
	g++ $(SRC) -lSDL2 -std=c++11 -o bin/snake_astar

Darwin: $(SRC)
	mkdir -p bin
	g++ $(SRC) -F/Library/Frameworks -framework SDL2 -std=c++11 -o bin/snake_astar

