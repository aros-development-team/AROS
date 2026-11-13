
CC	= c++
CCFLAGS = -Wall -Wformat -g -DDEBUG
HEADERS = be_val be_ptr be_conv aros_types.h Makefile

all:	test test-1 test-2

test:	test.c $(HEADERS)
	$(CC) $(CCFLAGS) test.c -o test

test-1: test-1.c $(HEADERS)
	$(CC) $(CCFLAGS) test-1.c -o test-1

test-2:	test-2.c $(HEADERS)
	$(CC) $(CCFLAGS) test-2.c -o test-2

clean:
	$(RM) -f test test-1 test-2
