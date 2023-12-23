CC=gcc
LDFLAGS=-lraylib -L../raylib/src
IFLAGS=-I../raylib/src -Isrc
CCFLAGS=-Wall -g -pipe

SOURCES=src/main.c contrib/gnu/getdelim.c

kotorpix:
	$(CC) $(LDFLAGS) $(IFLAGS) $(CCFLAGS) -o bin/kotorpx $(SOURCES)