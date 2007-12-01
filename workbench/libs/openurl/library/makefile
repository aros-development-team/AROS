TARGET  = openurl.library
DEST    = LIBS:

CC 	= ppc-morphos-gcc
CDEFS   = -DUSE_INLINE_STDARG
CFLAGS  = $(CDEFS) $(CWARN) -O2 -fomit-frame-pointer -mcpu=750 -mstring -mmultiple -noixemul -fstrict-aliasing -I../include
LDFLAGS = -noixemul -nostartfiles
CWARN   = -Wall\
	  -Wno-missing-braces\
	  -Wno-parentheses\
          -Wno-switch\
          -Wstrict-prototypes\
	  -Werror-implicit-function-declaration\
          -Wpointer-arith

OBJS	= libinit.o init.o api.o handler.o prefs.o utils.o morphos.o

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -nostartfiles -o $@ $(OBJS)
	ppc-morphos-strip $@

install:
	@copy $(TARGET) $(DEST)
	@delete $(TARGET)
	@avail flush

libinit.o: libinit.c lib.h openurl.library_rev.h base.h lib_protos.h
init.o:    init.c lib.h base.h lib_protos.h
api.o:     api.c lib.h openurl.library_rev.h base.h lib_protos.h
handler.o: handler.c lib.h base.h lib_protos.h
prefs.o:   prefs.c lib.h base.h lib_protos.h
utils.o:   utils.c lib.h base.h lib_protos.h
morphos.o: morphos.c lib.h openurl.library_rev.h base.h lib_protos.h

