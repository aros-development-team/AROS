
TOP = ../../..
MIC = $(TOP)/scripts/moveifchanged

CC = gcc
CFLAGS = -g -Wall -O2

SRCS = gram.c scan.c main.c emit.c emit_html.c varstr.c hash.c util.c exec.c
OBJS = $(subst .c,.o,$(SRCS))

.PHONY : makefile

.SUFFIXES :
.SUFFIXES : .c .h .y .l .skel

.PHONY : all
all : grammar src2html

src2html : $(OBJS) $(LIBS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY : grammar
grammar : gram.tab.c

gram.tab.c : gram.y gram.skel
	export BISON_SIMPLE=./gram.skel ; \
	bison --file-prefix=gram --defines --debug gram.y
	$(MIC) gram.tab.c gram.c
	$(MIC) gram.tab.h gram.h

scan.c : scan.l scan.skel
	flex -S./scan.skel -o$@ $<

.PHONY : test
test : src2html
	export TOP=$(TOP) ; \
	for file in *.src ; do \
	    src2html --html $$file --output=$$file.html ; \
	done

.PHONY : depend
depend :
	makedepend -- $(CFLAGS) -- $(SRCS)

.PRECIOUS : gram.c gram.h

# DO NOT DELETE
gram.o: error.h emit.h util.h 
scan.o: util.h
emit.o: hash.h error.h
emit.o: exec.h util.h
emit_html.o: gram.h emit.h 
varstr.o: varstr.h 
hash.o: hash.h 
util.o: util.h 
exec.o: exec.h error.h 
