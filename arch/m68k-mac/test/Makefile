## Makefile for LCD test
#TOP = ../../..
#include $(TOP)/config/make.cfg
#AROS_TARGET_ARCH = 

OBJS = $(TARGET).o
#LIBS = -L$(LIBDIR) -lexec -larossupport -larosc -lautoinit -lgcc1 
CC = m68k-elf-gcc
AS = m68k-elf-as

NM = m68k-elf-nm

all:  
	$(CC) $(CFLAGS) test.c -o test1.elf -O2 -ffreestanding -nostdlib -fomit-frame-pointer -nostartfiles -m68040 -Wl,-emain,-Ttext=0x1000,-Map=test.map,-nostdlib $(LIBS)
	! $(NM) -u test1.elf | grep .
	emile-install -f /lib/emile/first_floppy -s /lib/emile/m68k-second_floppy -k test1.elf test.img

depend:
	makedepend -Y -I. *.c

clean:
	-rm -f *.o $(TARGET).elf

veryclean: clean
	-rm -f $(TARGET).elf

