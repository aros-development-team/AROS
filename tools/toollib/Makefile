
PURIFY	= /home/digulla/AROS/compiler/purify/purify
CC	= gcc
CFLAGS	= -g -Wall -O -I..
AR	= ar
ARFLAGS = ru

libtoollib.a : toollib.o vstring.o hash.o error.o stdiocb.o stringcb.o \
	    mystream.o
	$(AR) $(ARFLAGS) $@ $?

%.o : %.c
	$(CC) $(CFLAGS) -c $<

pure :
	-rm *.o
	make "CC=$(PURIFY) $(CC)"

clean :
	-rm *.o lib*.a

depend :
	makedepend -- $(CFLAGS) -- *.c

# DO NOT DELETE

error.o: /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h
error.o: /usr/include/sys/cdefs.h /usr/include/errno.h
error.o: /usr/include/features.h /usr/include/linux/errno.h
error.o: /usr/include/asm/errno.h /usr/include/string.h /usr/include/stddef.h
error.o: ../toollib/error.h ../toollib/toollib.h /usr/include/sys/types.h
error.o: /usr/include/linux/types.h /usr/include/linux/posix_types.h
error.o: /usr/include/asm/posix_types.h /usr/include/asm/types.h
error.o: /usr/include/sys/bitypes.h /usr/include/stdarg.h
error.o: /usr/include/stdlib.h /usr/include/alloca.h
hash.o: /usr/include/string.h /usr/include/features.h
hash.o: /usr/include/sys/cdefs.h /usr/include/stddef.h /usr/include/stdlib.h
hash.o: /usr/include/errno.h /usr/include/linux/errno.h
hash.o: /usr/include/asm/errno.h /usr/include/alloca.h ../toollib/hash.h
hash.o: ../toollib/toollib.h /usr/include/sys/types.h
hash.o: /usr/include/linux/types.h /usr/include/linux/posix_types.h
hash.o: /usr/include/asm/posix_types.h /usr/include/asm/types.h
hash.o: /usr/include/sys/bitypes.h /usr/include/stdarg.h
hash.o: ../toollib/callback.h
mystream.o: /usr/include/string.h /usr/include/features.h
mystream.o: /usr/include/sys/cdefs.h /usr/include/stddef.h
mystream.o: ../toollib/mystream.h /usr/include/stdio.h /usr/include/libio.h
mystream.o: /usr/include/_G_config.h ../toollib/toollib.h
mystream.o: /usr/include/sys/types.h /usr/include/linux/types.h
mystream.o: /usr/include/linux/posix_types.h /usr/include/asm/posix_types.h
mystream.o: /usr/include/asm/types.h /usr/include/sys/bitypes.h
mystream.o: /usr/include/stdarg.h /usr/include/stdlib.h /usr/include/errno.h
mystream.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
mystream.o: /usr/include/alloca.h ../toollib/callback.h
stdiocb.o: /usr/include/string.h /usr/include/features.h
stdiocb.o: /usr/include/sys/cdefs.h /usr/include/stddef.h ../toollib/error.h
stdiocb.o: ../toollib/toollib.h /usr/include/sys/types.h
stdiocb.o: /usr/include/linux/types.h /usr/include/linux/posix_types.h
stdiocb.o: /usr/include/asm/posix_types.h /usr/include/asm/types.h
stdiocb.o: /usr/include/sys/bitypes.h /usr/include/stdarg.h
stdiocb.o: /usr/include/stdlib.h /usr/include/errno.h
stdiocb.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
stdiocb.o: /usr/include/alloca.h ../toollib/stdiocb.h /usr/include/stdio.h
stdiocb.o: /usr/include/libio.h /usr/include/_G_config.h
stdiocb.o: ../toollib/mystream.h ../toollib/callback.h
stringcb.o: ../toollib/error.h ../toollib/toollib.h /usr/include/sys/types.h
stringcb.o: /usr/include/linux/types.h /usr/include/linux/posix_types.h
stringcb.o: /usr/include/asm/posix_types.h /usr/include/asm/types.h
stringcb.o: /usr/include/sys/bitypes.h /usr/include/stdarg.h
stringcb.o: /usr/include/stdlib.h /usr/include/features.h
stringcb.o: /usr/include/sys/cdefs.h /usr/include/stddef.h
stringcb.o: /usr/include/errno.h /usr/include/linux/errno.h
stringcb.o: /usr/include/asm/errno.h /usr/include/alloca.h
stringcb.o: ../toollib/stringcb.h ../toollib/mystream.h /usr/include/stdio.h
stringcb.o: /usr/include/libio.h /usr/include/_G_config.h
stringcb.o: ../toollib/callback.h ../toollib/vstring.h
toollib.o: /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h
toollib.o: /usr/include/sys/cdefs.h /usr/include/stdlib.h
toollib.o: /usr/include/features.h /usr/include/stddef.h /usr/include/errno.h
toollib.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
toollib.o: /usr/include/alloca.h /usr/include/string.h /usr/include/assert.h
toollib.o: ../toollib/toollib.h /usr/include/sys/types.h
toollib.o: /usr/include/linux/types.h /usr/include/linux/posix_types.h
toollib.o: /usr/include/asm/posix_types.h /usr/include/asm/types.h
toollib.o: /usr/include/sys/bitypes.h /usr/include/stdarg.h
vstring.o: /usr/include/string.h /usr/include/features.h
vstring.o: /usr/include/sys/cdefs.h /usr/include/stddef.h
vstring.o: /usr/include/ctype.h /usr/include/endian.h /usr/include/bytesex.h
vstring.o: ../toollib/vstring.h
