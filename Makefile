CXX ?= c++
CXXFLAGS ?= -O3 -march=native -std=c++17 -Wall -Wextra -Wpedantic

.PHONY: all run check clean

all: program

program: main.cpp Matrix.hpp
	$(CXX) $(CXXFLAGS) main.cpp -o $@

run: program
	./program

check: program
	./program --check

clean:
	rm -f program
