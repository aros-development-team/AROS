PURIFY	= /home/digulla/AROS/compiler/purify/purify
CC	 = gcc
#CC	  = $(PURIFY) gcc
CFLAGS	 = -g -Wall -O -I.. -DHAVE_VSNPRINTF
DEPLIBS  = ../toollib/libtoollib.a
LIBS	 = -L../toollib -ltoollib
FORMAT	 = ./format
HPP	 = ./hpp
HPPFLAGS = -set format=$(FORMAT)

OBJS	 = parse_html.o html.o main.o var.o db.o html_filter.o expr.o func.o

FOBJS	 = format.o db.o var.o

all : $(HPP) format

$(HPP) : $(OBJS) $(DEPLIBS)
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

test : all ctest.html background.html background.ps ctest.ps AddHead.c \
	lists.c lists.html

%.html : srcs/%.src $(HPP) tohtml.inc
	$(HPP) $(HPPFLAGS) tohtml.inc $< > $@

lists.html : srcs/lists.src $(HPP)
	$(HPP) $(HPPFLAGS) -set functiontmpl=amigafunc.tmpl tohtml.inc $< -o $@

AddHead.c : srcs/lists.src toc.inc ssrc.tmpl $(HPP)
	$(HPP) $(HPPFLAGS) -set functiontmpl=ssrc.tmpl toc.inc $< -o $@

lists.c : srcs/lists.src toc.inc msrc.tmpl $(HPP)
	-rm lists.h
	$(HPP) $(HPPFLAGS) -set functiontmpl=msrc.tmpl toc.inc $< -o $@
	sed -f toc.filter lists.h > lists.tmp
	mv lists.tmp lists.h

ctest.html : srcs/ctest.c ctohtml tohtml.inc $(HPP)
	ctohtml $<

ctest.tex : srcs/ctest.c ctolatex tolatex.inc $(HPP) ctolatex.filter
	ctolatex $<

%.html : %.c ctohtml $(HPP) tohtml.inc
	ctohtml $< > $@

%.tex : srcs/%.src $(HPP) tolatex.inc
	$(HPP) $(HPPFLAGS) tolatex.inc $< > $@

%.dvi : %.tex
	latex $<
	latex $<
	latex $<

%.ps : %.dvi
	dvips $<

../toollib/libtoollib.a :
	cd ../toollib ; make

clean :
	rm -f *.o $(FORMAT) $(HPP) htmlparsetest *.html *.bak *.log *.tex *.aux \
		*.ps *.cb lists.[ch]

depend :
	makedepend -- $(CFLAGS) -- *.c

