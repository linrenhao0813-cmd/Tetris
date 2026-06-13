CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
LDFLAGS := -lncursesw

tetris: main.o tetris.o
	$(CXX) -o $@ $^ $(LDFLAGS)

main.o: main.cpp tetris.h
tetris.o: tetris.cpp tetris.h

clean:
	rm -f tetris *.o

.PHONY: clean
