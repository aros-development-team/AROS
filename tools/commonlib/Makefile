
CC     = gcc
CFLAGS = -Wall -g -O2 -I. -I.. -I../../bin/linux-i386/gen/include

test : setup test/strlen
	test/strlen

setup :
	if [ ! -d test ]; then mkdir test ; else true ; fi

test/strlen : strlen.c strlen.h commonlib.h
	$(CC) $(CFLAGS) $< -o $@
