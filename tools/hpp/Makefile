PURIFY	= /home/digulla/AROS/compiler/purify/purify
CC	 = gcc
#CC	  = $(PURIFY) gcc
CFLAGS	 = -g -Wall -O -I.. -DHAVE_VSNPRINTF
DEPLIBS  = ../toollib/libtoollib.a
LIBS	 = -L../toollib -ltoollib
FORMAT	 = ./format
HPP	 = ./hpp
HPPFLAGS = -set format=$(FORMAT)

OBJS	 = parse_html.o html.o main.o var.o db.o html_filter.o expr.o \
	    func.o img.o

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

background.dvi : pics/aros_logo.eps pics/demowin.eps pics/readicon.eps

pics/%.eps : pics/%
	anytopnm $< | pnmtops -noturn -nocenter > $@

../toollib/libtoollib.a :
	cd ../toollib ; make

clean :
	rm -f *.o $(FORMAT) $(HPP) htmlparsetest *.html *.bak *.log *.tex *.aux \
		*.ps *.cb lists.[ch] pics/*.eps

depend :
	makedepend -- $(CFLAGS) -- *.c

# DO NOT DELETE

db.o: /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h
db.o: /usr/include/sys/cdefs.h ../toollib/error.h ../toollib/toollib.h
db.o: /usr/include/sys/types.h /usr/include/linux/types.h
db.o: /usr/include/linux/posix_types.h /usr/include/asm/posix_types.h
db.o: /usr/include/asm/types.h /usr/include/sys/bitypes.h
db.o: /usr/include/stdarg.h /usr/include/stdlib.h /usr/include/features.h
db.o: /usr/include/stddef.h /usr/include/errno.h /usr/include/linux/errno.h
db.o: /usr/include/asm/errno.h /usr/include/alloca.h ../toollib/hash.h
db.o: ../toollib/callback.h db.h var.h ../toollib/vstring.h
expr.o: /usr/include/ctype.h /usr/include/features.h /usr/include/sys/cdefs.h
expr.o: /usr/include/endian.h /usr/include/bytesex.h ../toollib/error.h
expr.o: ../toollib/toollib.h /usr/include/sys/types.h
expr.o: /usr/include/linux/types.h /usr/include/linux/posix_types.h
expr.o: /usr/include/asm/posix_types.h /usr/include/asm/types.h
expr.o: /usr/include/sys/bitypes.h /usr/include/stdarg.h
expr.o: /usr/include/stdlib.h /usr/include/stddef.h /usr/include/errno.h
expr.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
expr.o: /usr/include/alloca.h expr.h ../toollib/vstring.h parse.h
expr.o: /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h
expr.o: var.h
format.o: /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h
format.o: /usr/include/sys/cdefs.h /usr/include/string.h
format.o: /usr/include/features.h /usr/include/stddef.h /usr/include/ctype.h
format.o: /usr/include/endian.h /usr/include/bytesex.h ../toollib/error.h
format.o: ../toollib/toollib.h /usr/include/sys/types.h
format.o: /usr/include/linux/types.h /usr/include/linux/posix_types.h
format.o: /usr/include/asm/posix_types.h /usr/include/asm/types.h
format.o: /usr/include/sys/bitypes.h /usr/include/stdarg.h
format.o: /usr/include/stdlib.h /usr/include/errno.h
format.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
format.o: /usr/include/alloca.h ../toollib/vstring.h db.h var.h
func.o: /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h
func.o: /usr/include/sys/cdefs.h /usr/include/string.h
func.o: /usr/include/features.h /usr/include/stddef.h ../toollib/error.h
func.o: ../toollib/toollib.h /usr/include/sys/types.h
func.o: /usr/include/linux/types.h /usr/include/linux/posix_types.h
func.o: /usr/include/asm/posix_types.h /usr/include/asm/types.h
func.o: /usr/include/sys/bitypes.h /usr/include/stdarg.h
func.o: /usr/include/stdlib.h /usr/include/errno.h /usr/include/linux/errno.h
func.o: /usr/include/asm/errno.h /usr/include/alloca.h var.h
func.o: ../toollib/vstring.h parse.h expr.h
html.o: /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h
html.o: /usr/include/sys/cdefs.h /usr/include/string.h
html.o: /usr/include/features.h /usr/include/stddef.h /usr/include/ctype.h
html.o: /usr/include/endian.h /usr/include/bytesex.h ../toollib/toollib.h
html.o: /usr/include/sys/types.h /usr/include/linux/types.h
html.o: /usr/include/linux/posix_types.h /usr/include/asm/posix_types.h
html.o: /usr/include/asm/types.h /usr/include/sys/bitypes.h
html.o: /usr/include/stdarg.h /usr/include/stdlib.h /usr/include/errno.h
html.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
html.o: /usr/include/alloca.h ../toollib/mystream.h ../toollib/callback.h
html.o: ../toollib/stdiocb.h ../toollib/stringcb.h ../toollib/vstring.h
html.o: ../toollib/error.h parse.h parse_html.h var.h html.h expr.h main.h
html.o: img.h
html_filter.o: /usr/include/unistd.h /usr/include/features.h
html_filter.o: /usr/include/sys/cdefs.h /usr/include/posix_opt.h
html_filter.o: /usr/include/gnu/types.h /usr/include/stddef.h
html_filter.o: /usr/include/confname.h /usr/include/sys/types.h
html_filter.o: /usr/include/linux/types.h /usr/include/linux/posix_types.h
html_filter.o: /usr/include/asm/posix_types.h /usr/include/asm/types.h
html_filter.o: /usr/include/sys/bitypes.h ../toollib/error.h
html_filter.o: ../toollib/toollib.h /usr/include/stdarg.h
html_filter.o: /usr/include/stdlib.h /usr/include/errno.h
html_filter.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
html_filter.o: /usr/include/alloca.h ../toollib/stdiocb.h
html_filter.o: /usr/include/stdio.h /usr/include/libio.h
html_filter.o: /usr/include/_G_config.h ../toollib/stringcb.h
html_filter.o: ../toollib/vstring.h html.h parse.h var.h
img.o: /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h
img.o: /usr/include/sys/cdefs.h /usr/include/string.h /usr/include/features.h
img.o: /usr/include/stddef.h ../toollib/error.h ../toollib/toollib.h
img.o: /usr/include/sys/types.h /usr/include/linux/types.h
img.o: /usr/include/linux/posix_types.h /usr/include/asm/posix_types.h
img.o: /usr/include/asm/types.h /usr/include/sys/bitypes.h
img.o: /usr/include/stdarg.h /usr/include/stdlib.h /usr/include/errno.h
img.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
img.o: /usr/include/alloca.h img.h
main.o: /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h
main.o: /usr/include/sys/cdefs.h /usr/include/time.h /usr/include/features.h
main.o: /usr/include/sys/time.h /usr/include/linux/types.h
main.o: /usr/include/linux/posix_types.h /usr/include/asm/posix_types.h
main.o: /usr/include/asm/types.h /usr/include/linux/time.h
main.o: /usr/include/sys/types.h /usr/include/sys/bitypes.h
main.o: /usr/include/errno.h /usr/include/linux/errno.h
main.o: /usr/include/asm/errno.h /usr/include/ctype.h /usr/include/endian.h
main.o: /usr/include/bytesex.h ../toollib/stdiocb.h ../toollib/error.h
main.o: ../toollib/toollib.h /usr/include/stdarg.h /usr/include/stdlib.h
main.o: /usr/include/stddef.h /usr/include/alloca.h html.h parse.h var.h
main.o: ../toollib/vstring.h db.h func.h main.h
parse_html.o: /usr/include/stdio.h /usr/include/libio.h
parse_html.o: /usr/include/_G_config.h /usr/include/sys/cdefs.h
parse_html.o: /usr/include/ctype.h /usr/include/features.h
parse_html.o: /usr/include/endian.h /usr/include/bytesex.h
parse_html.o: /usr/include/assert.h /usr/include/string.h
parse_html.o: /usr/include/stddef.h parse_html.h parse.h ../toollib/hash.h
parse_html.o: ../toollib/callback.h ../toollib/error.h ../toollib/toollib.h
parse_html.o: /usr/include/sys/types.h /usr/include/linux/types.h
parse_html.o: /usr/include/linux/posix_types.h /usr/include/asm/posix_types.h
parse_html.o: /usr/include/asm/types.h /usr/include/sys/bitypes.h
parse_html.o: /usr/include/stdarg.h /usr/include/stdlib.h
parse_html.o: /usr/include/errno.h /usr/include/linux/errno.h
parse_html.o: /usr/include/asm/errno.h /usr/include/alloca.h
var.o: /usr/include/ctype.h /usr/include/features.h /usr/include/sys/cdefs.h
var.o: /usr/include/endian.h /usr/include/bytesex.h /usr/include/stdio.h
var.o: /usr/include/libio.h /usr/include/_G_config.h ../toollib/error.h
var.o: ../toollib/toollib.h /usr/include/sys/types.h
var.o: /usr/include/linux/types.h /usr/include/linux/posix_types.h
var.o: /usr/include/asm/posix_types.h /usr/include/asm/types.h
var.o: /usr/include/sys/bitypes.h /usr/include/stdarg.h /usr/include/stdlib.h
var.o: /usr/include/stddef.h /usr/include/errno.h /usr/include/linux/errno.h
var.o: /usr/include/asm/errno.h /usr/include/alloca.h var.h
var.o: ../toollib/vstring.h parse.h
