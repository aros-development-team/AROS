
TOP = ../../..
MIC = $(TOP)/scripts/moveifchanged

CC = cc
CFLAGS = -g -Wall -O

SRCS = gram.c scan.c main.c emit.c emit_html.c varstr.c hash.c util.c exec.c
OBJS = $(subst .c,.o,$(SRCS))

all : grammar src2html

src2html : $(OBJS) $(LIBS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

grammar : gram.tab.c

gram.tab.c : gram.y gram.skel
	export BISON_SIMPLE=./gram.skel ; \
	bison --file-prefix=gram --defines --debug gram.y
	$(MIC) gram.tab.c gram.c
	$(MIC) gram.tab.h gram.h

scan.c : scan.l scan.skel
	flex -S./scan.skel -o$@ $<

test : src2html
	export TOP=$(TOP) ; \
	for file in *.src ; do \
	    src2html --html $$file --output=$$file.html ; \
	done

depend :
	makedepend -- $(CFLAGS) -- $(SRCS)

# DO NOT DELETE

gram.o: /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h
gram.o: /usr/include/sys/cdefs.h /usr/include/stdlib.h
gram.o: /usr/include/features.h /usr/include/stddef.h /usr/include/errno.h
gram.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
gram.o: /usr/include/alloca.h /usr/include/string.h error.h emit.h util.h
scan.o: /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h
scan.o: /usr/include/sys/cdefs.h gram.h error.h varstr.h
scan.o: /usr/include/stdarg.h /usr/include/string.h /usr/include/features.h
scan.o: /usr/include/stddef.h util.h emit.h scan.h /usr/include/stdlib.h
scan.o: /usr/include/errno.h /usr/include/linux/errno.h
scan.o: /usr/include/asm/errno.h /usr/include/alloca.h
main.o: emit.h /usr/include/stdio.h /usr/include/libio.h
main.o: /usr/include/_G_config.h /usr/include/sys/cdefs.h
main.o: /usr/include/string.h /usr/include/features.h /usr/include/stddef.h
main.o: /usr/include/stdlib.h /usr/include/errno.h /usr/include/linux/errno.h
main.o: /usr/include/asm/errno.h /usr/include/alloca.h
emit.o: /usr/include/errno.h /usr/include/features.h /usr/include/sys/cdefs.h
emit.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
emit.o: /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h
emit.o: /usr/include/stdlib.h /usr/include/stddef.h /usr/include/alloca.h
emit.o: /usr/include/string.h /usr/include/sys/stat.h
emit.o: /usr/include/sys/types.h /usr/include/linux/types.h
emit.o: /usr/include/linux/posix_types.h /usr/include/asm/posix_types.h
emit.o: /usr/include/asm/types.h /usr/include/sys/bitypes.h
emit.o: /usr/include/linux/stat.h /usr/include/unistd.h
emit.o: /usr/include/posix_opt.h /usr/include/gnu/types.h
emit.o: /usr/include/confname.h /usr/include/time.h /usr/include/sys/time.h
emit.o: /usr/include/linux/time.h /usr/include/ctype.h /usr/include/endian.h
emit.o: /usr/include/bytesex.h gram.h emit.h emit_intern.h
emit.o: /usr/include/stdarg.h hash.h error.h exec.h util.h
emit_html.o: gram.h emit.h /usr/include/stdio.h /usr/include/libio.h
emit_html.o: /usr/include/_G_config.h /usr/include/sys/cdefs.h emit_intern.h
emit_html.o: /usr/include/stdarg.h error.h /usr/include/ctype.h
emit_html.o: /usr/include/features.h /usr/include/endian.h
emit_html.o: /usr/include/bytesex.h /usr/include/string.h
emit_html.o: /usr/include/stddef.h scan.h
varstr.o: varstr.h /usr/include/string.h /usr/include/features.h
varstr.o: /usr/include/sys/cdefs.h /usr/include/stddef.h
varstr.o: /usr/include/stdlib.h /usr/include/errno.h
varstr.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
varstr.o: /usr/include/alloca.h util.h
hash.o: hash.h /usr/include/string.h /usr/include/features.h
hash.o: /usr/include/sys/cdefs.h /usr/include/stddef.h /usr/include/stdlib.h
hash.o: /usr/include/errno.h /usr/include/linux/errno.h
hash.o: /usr/include/asm/errno.h /usr/include/alloca.h util.h
util.o: /usr/include/pwd.h /usr/include/features.h /usr/include/sys/cdefs.h
util.o: /usr/include/gnu/types.h /usr/include/stdio.h /usr/include/libio.h
util.o: /usr/include/_G_config.h /usr/include/sys/types.h
util.o: /usr/include/linux/types.h /usr/include/linux/posix_types.h
util.o: /usr/include/asm/posix_types.h /usr/include/asm/types.h
util.o: /usr/include/sys/bitypes.h /usr/include/stdlib.h
util.o: /usr/include/stddef.h /usr/include/errno.h /usr/include/linux/errno.h
util.o: /usr/include/asm/errno.h /usr/include/alloca.h /usr/include/string.h
util.o: /usr/include/ctype.h /usr/include/endian.h /usr/include/bytesex.h
util.o: error.h varstr.h util.h
