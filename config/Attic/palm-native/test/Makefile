TOP = ../../..

INCDIR = $(TOP)/bin/linux-i386/AROS/Include
ARCH = -mc68000 -Wa,-mc68000

all : test test2

test.o : test.c
	m68k-linux-gcc -c test.c -O0 -I $(INCDIR) $(ARCH)

test : test.o
	m68k-linux-gcc -nostartfiles test.o -o test -Wl,-r -L./ -lgcc1 -dp $(ARCH)

clean:
	rm -rf *.o test
