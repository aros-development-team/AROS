PURIFY	= /home/digulla/AROS/compiler/purify/purify
CC	 = gcc
#CC	  = $(PURIFY) gcc
CFLAGS	 = -g -Wall -O -I.. -DHAVE_VSNPRINT
DEPLIBS  = ../toollib/libtoollib.a
LIBS	 = -L../toollib -ltoollib

OBJS	 = parse_html.o html.o main.o var.o db.o html_filter.o expr.o func.o

FOBJS	 = format.o db.o var.o

all : hpp format

hpp : $(OBJS) $(DEPLIBS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LIBS)

format : $(FOBJS)
	$(CC) $(CFLAGS) $(FOBJS) -o $@ $(LIBS)

htmlparsetest : test_parse_html.o $(DEPLIBS)
	$(CC) $(CFLAGS) test_parse_html.o -o $@ $(LIBS)

exprtest : test_expr.o $(DEPLIBS)
	$(CC) $(CFLAGS) test_expr.o -o $@ $(LIBS)

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

test_%.o : %.c
	$(CC) -DTEST $(CFLAGS) -c $< -o $@

test : test.html background.html background.ps ctest.ps

%.html : srcs/%.src
	hpp tohtml.inc $< > $@

ctest.html : srcs/ctest.c ctohtml tohtml.inc
	ctohtml $<

ctest.tex : srcs/ctest.c ctolatex tolatex.inc
	ctolatex $<

%.html : %.c
	ctohtml $< > $@

%.tex : srcs/%.src
	hpp tolatex.inc $< > $@

%.dvi : %.tex
	latex $<
	latex $<
	latex $<

%.ps : %.dvi
	dvips $<

../toollib/libtoollib.a :
	cd ../toollib ; make

clean :
	rm -f *.o format hpp htmlparsetest *.html *.bak *.log *.tex *.aux

depend :
	makedepend -- $(CFLAGS) -- *.c

