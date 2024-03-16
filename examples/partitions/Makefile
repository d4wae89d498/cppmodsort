MODS=$(shell ../../cppmodsort $(shell ls *.cppm | sort -r)) # sort -r to make sure its firstly in wrong order
SRCS=$(shell ls *.cpp)

PCMS=$(MODS:.cppm=.pcm)
OBJS=$(MODS:.cppm=.o) $(SRCS:.cpp=.o)

CC=clang++ -std=c++20 -fprebuilt-module-path=.

.SUFFIXES:
.PRECIOUS:  $(PCMS)
.PHONY: 	all clean re

all: main

%.pcm: %.cppm
	$(CC) --precompile $< -o $@

%.o: %.cppm %.pcm
	$(CC) -c $< -o $@

%.o: %.cpp
	$(CC) -c $< -o $@

main: $(OBJS)
	$(CC) $(OBJS) -o $@
	echo "BUILD ORDER OK"
clean:
	rm -rf main $(OBJS) $(PCMS)

re: clean all
