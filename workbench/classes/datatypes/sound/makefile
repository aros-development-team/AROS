
#
# Makefile for sound.datatype!
#

CC		= gcc
CFLAGS		= -m68020-60 -O2 -fstrength-reduce -fomit-frame-pointer -msmall-code -c
LDFLAGS		= -nostartfiles -noixemul -nostdlib
OBJS		= library.o dispatch.o

v41sound.datatype:	$(OBJS)
		$(CC) $(LDFLAGS) -o SYS:Classes/DataTypes/$@ $(OBJS)

library.o:	library.c classbase.h
		$(CC) $(CFLAGS) $< -fno-force-mem

dispatch.o:	dispatch.c classbase.h aiff.h
		$(CC) $(CFLAGS) $< 

clean:
		-Delete $(OBJS)
