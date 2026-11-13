# Makefile for dtdesc

# .SUFFIXES: .c.o

CC = cc

CFLAGS = -I . -I c_iff/

RM = rm -f
AR = ar rv
RANLIB = ranlib

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

# .c.o:
#	$(CC) $(CFLAGS) -c $*.c -o $@

all : createdtdesc examinedtdesc

createdtdesc : createdtdesc.o c_iff/libc_iff.a
	$(CC) -o createdtdesc createdtdesc.o -L c_iff/ -l c_iff

examinedtdesc : examinedtdesc.o c_iff/libc_iff.a
	$(CC) -o examinedtdesc examinedtdesc.o -L c_iff/ -l c_iff

c_iff/libc_iff.a :
	cd c_iff && \
	make

clean :
	$(RM) *.o
	$(RM) examinedtdesc
	$(RM) createdtdesc
	cd c_iff && \
	make clean


