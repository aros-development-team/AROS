CC	= gcc
CFLAGS	= -DLC_LIBDEFS_FILE=\"libdefs.h\" -O2 -fstrength-reduce -fomit-frame-pointer -c #-m68020-40 -mregparm=3 -msmall-code
LDFLAGS	= -noixemul -nostartfiles -s
LDLIBS	=

OBJS	= library.o support.o colorwheelclass.o converthsbtorgb.o convertrgbtohsb.o fixmathtrans.o bmbmrp.o

colorwheel.gadget: $(OBJS)
	$(CC) $(LDFLAGS) -o sys:classes/gadgets/$@ $(OBJS) $(LDLIBS)
	avail flush >NIL:

library.o: library.c colorwheel_intern.h libdefs.h
	$(CC) $(CFLAGS) library.c

colorwheelclass.o: colorwheelclass.c colorwheel_intern.h
	$(CC) $(CFLAGS) colorwheelclass.c
	
converthsbtorgb.o: converthsbtorgb.c
	$(CC) $(CFLAGS) converthsbtorgb.c
	
convertrgbtohsb.o: convertrgbtohsb.c fixmath.h
	$(CC) $(CFLAGS) convertrgbtohsb.c

support.o: support.c colorwheel_intern.h fixmath.h
	$(CC) $(CFLAGS) support.c
	
bmbmrp.o: bmbmrp.c bmbmrp.h
	$(CC) $(CFLAGS) bmbmrp.c
	
fixmathtrans.o:	fixmathtrans.c fixmath.h
	$(CC) $(CFLAGS) fixmathtrans.c
