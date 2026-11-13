# remove the -DDEBUG from CFLAGS if not debugging
CFLAGS	=
#	   -DDEBUG

# set the DEBUG variables to null strings if not debugging
DEBUG_H  =
#	    pipedebug.h
DEBUG_O  =
#	    pipedebug.o


PIPEHDR  =  pipe-handler.h pipelists.h pipename.h pipebuf.h \
pipecreate.h pipesched.h pipedir.h

OBJ	 =  pipe-handler.o pipelists.o pipename.o pipebuf.o \
pipecreate.o pipesched.o pipedir.o


mount : loader_mount

handler : loader_pipe-handler

obj : $(OBJ) $(DEBUG_O)


prelude_mount : prelude_pipe-handler
    Copy  prelude_pipe-handler	L:pipe-handler
    Copy  prelude_Mount		DEVS:Mountlist
    Mount P:

loader_mount : loader loader_pipe-handler
    Copy  pipe-handler-loader	L:
    Copy  loader_pipe-handler	L:pipe-handler
    Copy  loader_Mount		DEVS:Mountlist
    Mount P:


prelude_pipe-handler : prelude.o $(OBJ) $(DEBUG_O)
    BLink FROM	   prelude.o $(OBJ) $(DEBUG_O) \
	  TO	   prelude_pipe-handler \
	  LIBRARY  CLIB:lc.lib CLIB:amiga.lib


loader_pipe-handler : $(OBJ) $(DEBUG_O)
    BLink FROM	   $(OBJ) $(DEBUG_O) \
	  TO	   loader_pipe-handler \
	  LIBRARY  CLIB:lc.lib CLIB:amiga.lib


prelude : prelude.o

prelude.o : prelude.asm
    Assem prelude  -o prelude.o


loader : loader.o
    BLink  FROM loader.o  TO pipe-handler-loader

loader.o : loader.asm
    Assem loader.asm  -i :include	-o loader.o  -c w100000


pipedebug.o : pipedebug.h pipedebug.c
    EXECUTE cc pipedebug $(CFLAGS)

pipe-handler.o : $(PIPEHDR) pipe-handler.c $(DEBUG_H)
    EXECUTE cc pipe-handler $(CFLAGS)

pipelists.o : pipelists.h pipelists.c
    EXECUTE cc pipelists $(CFLAGS)

pipename.o : pipe-handler.h pipelists.h pipename.h pipebuf.h
pipename.o : pipecreate.h pipesched.h
pipename.o : pipename.c
    EXECUTE cc pipename $(CFLAGS)

pipebuf.o : pipe-handler.h pipelists.h pipename.h pipebuf.h
pipebuf.o : pipecreate.h pipesched.h
pipebuf.o : pipebuf.c
    EXECUTE cc pipebuf $(CFLAGS)

pipecreate.o : $(PIPEHDR) pipecreate.c $(DEBUG_H)
    EXECUTE cc pipecreate $(CFLAGS)

pipesched.o : $(PIPEHDR) pipesched.c $(DEBUG_H)
    EXECUTE cc pipesched $(CFLAGS)

pipedir.o : $(PIPEHDR) pipedir.c
    EXECUTE cc pipedir $(CFLAGS)
