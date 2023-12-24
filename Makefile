CC=clang
LDFLAGS=../raylib/src/libraylib.a
IFLAGS=-I../raylib/src -Isrc
CCFLAGS=-Wall -g -pipe -Wno-deprecated-declarations -O0

SOURCES=src/main.c contrib/gnu/getdelim.c

kotorpix:
	$(CC) $(LDFLAGS) $(IFLAGS) $(CCFLAGS) -o bin/kotorpx.exe $(SOURCES)