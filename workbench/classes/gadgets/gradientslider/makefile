CC	= gcc
CFLAGS	= -mregparm=3 -O2 -fstrength-reduce -fomit-frame-pointer -msmall-code -c
LDFLAGS	= -noixemul -nostartfiles -s
LDLIBS	=

OBJS	= library.o support.o gradientsliderclass.o

gradientslider.gadget: $(OBJS)
	$(CC) $(LDFLAGS) -o sys:classes/gadgets/$@ $(OBJS) $(LDLIBS)
	avail flush >nil:

library.o: library.c gradientslider_intern.h libdefs.h
	$(CC) $(CFLAGS) library.c -m68020-60

gradientsliderclass.o: gradientsliderclass.c gradientslider_intern.h
	$(CC) $(CFLAGS) gradientsliderclass.c -m68020-60

support.o: support.c gradientslider_intern.h
	$(CC) $(CFLAGS) support.c -m68020-60

#integer.o:	integer.s
#		PhxAss $(PHXASSOPTS) $<
#		-Delete obj.#? QUIET
#		Hunk2aout $@
#		Delete $@
#		Rename `Dir obj.#?` AS $@