CC	 = gcc
CFLAGS	 = -g -Wall -O -I../toollib
DEPLIBS  = ../toollib/libtoollib.a
LIBS	 = -L../toollib -ltoollib

OBJS	 = parse_html.o html.o main.o

all : hpp

hpp : $(OBJS) $(DEPLIBS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LIBS)

htmlparsetest : test_parse_html.o $(DEPLIBS)
	$(CC) $(CFLAGS) test_parse_html.o -o $@ $(LIBS)

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

test_%.o : %.c
	$(CC) -DTEST $(CFLAGS) -c $< -o $@

parse_html.o : parse_html.h ../toollib/callback.h ../toollib/toollib.h \
	../toollib/hash.h ../toollib/vstring.h ../toollib/stdiocb.h
test_parse_html.o : parse_html.h ../toollib/callback.h ../toollib/toollib.h \
	../toollib/hash.h ../toollib/vstring.h ../toollib/stdiocb.h \
	../toollib/stringcb.h

