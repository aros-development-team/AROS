
PURIFY	= /home/digulla/AROS/compiler/purify/purify
CC	= gcc
CFLAGS	= -g -Wall -O -I.
AR	= ar
ARFLAGS = ru

libtoollib.a : toollib.o vstring.o hash.o error.o stdiocb.o stringcb.o
	$(AR) $(ARFLAGS) $@ $?

%.o : %.c
	$(CC) $(CFLAGS) -c $<

pure :
	-rm *.o
	make "CC=$(PURIFY) $(CC)"

clean :
	-rm *.o lib*.a

toollib.o : toollib.h
vstring.o : toollib.h vstring.h
hash.o : hash.h toollib.h
error.o : error.h toollib.h
stdiocb.o : stdiocb.h callback.h toollib.h error.h
stringcb.o : stringcb.h stdiocb.h callback.h toollib.h error.h

