PURIFY	= /home/digulla/AROS/compiler/purify/purify
CC	 = gcc
#CC	  = $(PURIFY) gcc
CFLAGS	 = -g -Wall -O -I..
DEPLIBS  = ../toollib/libtoollib.a
LIBS	 = -L../toollib -ltoollib

OBJS	 = parse_html.o html.o main.o var.o db.o html_filter.o

FOBJS	 = format.o db.o var.o

all : hpp format

hpp : $(OBJS) $(DEPLIBS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LIBS)

format : $(FOBJS)
	$(CC) $(CFLAGS) $(FOBJS) -o $@ $(LIBS)

htmlparsetest : test_parse_html.o $(DEPLIBS)
	$(CC) $(CFLAGS) test_parse_html.o -o $@ $(LIBS)

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

test_%.o : %.c
	$(CC) -DTEST $(CFLAGS) -c $< -o $@

test : test.html

%.html : %.src
	hpp $< > $@

../toollib/libtoollib.a :
	cd ../toollib ; make

clean :
	rm -f *.o format hpp htmlparsetest *.html *.bak

depend :
	makedepend -- $(CFLAGS) -- *.c

