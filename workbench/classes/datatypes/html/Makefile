#   $Id$
#   This Makefile creates a standalone application for debugging with console only

CFLAGS	+= -O2 #-Wall

OBJS = main.o tables.o parse.o layout.o

all: parser

parser: $(OBJS)
	gcc -o $@ $(OBJS)

$(OBJS): %.o: %.c %.h common.h
	gcc -c $(CFLAGS) $< -o $@

parse.o: tables.h

tables.o: parse.h

html.datatype: htmlclass.o tables.o parse.o layout.o
	gcc -o $@ htmlclass.o tables.o parse.o #layout.o

clean:
	rm -f *.o *.bak parser

install:
	copy html.datatype SYS:Classes/Datatypes/
