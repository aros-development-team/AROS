SHELL = /bin/sh

CFLAGS = -g -O4 -Wall
BASEDIR = $(DESTDIR)/usr
BINDIR  = $(BASEDIR)/bin


all:	icctrans wtpt

icctrans: icctrans.c xgetopt.c
	$(CC) $(CFLAGS) icctrans.c xgetopt.c ../src/liblcms.a -I../include -o icctrans -lm

wtpt: wtpt.c
	$(CC) $(CFLAGS) wtpt.c ../src/liblcms.a -I../include -o wtpt -lm

install: icctrans
	-cp icctrans $(BINDIR)
	-cp icctrans.exe $(BINDIR)  # Cygwin
	
test:
	# Nothing to do for test target

clean:
	-rm wtpt icctrans wtpt.exe icctrans.exe
