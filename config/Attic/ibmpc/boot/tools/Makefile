#
# Makefile to build "build" file
#

CC=gcc -O2 -D__BFD__

build: build.c
	$(CC) $< -o $@

clean:
	rm -f build
