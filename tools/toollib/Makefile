
PURIFY	= /home/digulla/AROS/compiler/purify/purify
CC	= gcc
CFLAGS	= -g -Wall -O -I.. -DHAVE_VSNPRINTF
AR	= ar
ARFLAGS = ru

libtoollib.a : toollib.o vstring.o hash.o error.o stdiocb.o stringcb.o \
	    mystream.o
	$(AR) $(ARFLAGS) $@ $?

%.o : %.c
	$(CC) $(CFLAGS) -c $<

pure :
	-rm *.o
	make "CC=$(PURIFY) $(CC)"

clean :
	-rm -f *.o lib*.a

depend :
	makedepend -- $(CFLAGS) -- *.c

