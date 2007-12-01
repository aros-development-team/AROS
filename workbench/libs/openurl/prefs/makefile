
CC 	= ppc-morphos-gcc
CDEFS   = -DUSE_INLINE_STDARG
CFLAGS  = $(CDEFS) $(CWARN) -O2 -fomit-frame-pointer -mcpu=750 -mstring -mmultiple -noixemul -fstrict-aliasing -I../include/
LDFLAGS = -noixemul
CWARN   = -Wall\
	  -Wno-missing-braces\
	  -Wno-parentheses\
          -Wno-switch\
          -Wstrict-prototypes\
	  -Werror-implicit-function-declaration\
          -Wpointer-arith

OBJS    = OpenURL.o loc.o utils.o stubs.o popport.o popph.o ftpeditwin.o mailereditwin.o browsereditwin.o applist.o win.o pens.o about.o app.o

OpenURL: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)
	ppc-morphos-strip $@

OpenURL.o: 	  OpenURL.c OpenURL.h loc.h OpenURL_protos.h
loc.o: 		  loc.c OpenURL.h loc.h OpenURL_protos.h
utils.o: 	  utils.c OpenURL.h OpenURL_protos.h
stubs.o: 	  stubs.c OpenURL.h OpenURL_protos.h
popport.o: 	  popport.c OpenURL.h OpenURL_protos.h
popph.o: 	  popph.c OpenURL.h OpenURL_protos.h
ftpeditwin.o: 	  ftpeditwin.c OpenURL.h loc.h OpenURL_protos.h
mailereditwin.o:  mailereditwin.c OpenURL.h loc.h OpenURL_protos.h
browsereditwin.o: browsereditwin.c OpenURL.h loc.h OpenURL_protos.h
applist.o: 	  applist.c OpenURL.h loc.h OpenURL_protos.h
win.o: 	     	  win.c OpenURL.h loc.h OpenURL_protos.h
pens.o: 	  pens.c OpenURL.h loc.h OpenURL_protos.h
about.o: 	  about.c OpenURL.h loc.h OpenURL_rev.h OpenURL_protos.h
app.o: 		  app.c OpenURL.h loc.h OpenURL_rev.h OpenURL_protos.h

