# Makefile for c_iff

.SUFFIXES: .c.o

CC = \
	cc

CFLAGS = -I .

RM = rm -f
AR = ar rv
RANLIB = ranlib

OBJS = \
	checkiff.o \
	closeiff.o \
	endchunk.o \
	filesize.o \
	fixiffsize.o \
	newchunk.o \
	newiff.o \
	newsubform.o \
	openiff.o \
	readchunkdata.o \
	readchunkheader.o \
	skipchunkdata.o \
	writechunkdata.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c -o $@

all: libc_iff.a
doc: c_iff.txt

libc_iff.a: $(OBJS)
	$(RM) libc_iff.a
	$(AR) $@ $(OBJS)
	$(RANLIB) $@

c_iff.txt:
	perl autodoc.pl *.c >c_iff.txt

example: example.o libc_iff.a hot.iff
	$(CC) $(CFLAGS) -o $@ example.o -L . -lc_iff

$(OBJS): c_iff.h

clean:
	$(RM) libc_iff.a
	$(RM) *.o
	$(RM) c_iff.txt
	$(RM) example
	$(RM) test1.iff

