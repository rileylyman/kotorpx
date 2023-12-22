CC=gcc
LDFLAGS=-lraylib -L../raylib/src
IFLAGS=-I../raylib/src -I.
CCFLAGS=-Wall -g -pipe

SOURCES=main.c

kotorpix:
	$(CC) $(LDFLAGS) $(IFLAGS) $(CCFLAGS) -o bin/kotorpx $(SOURCES)