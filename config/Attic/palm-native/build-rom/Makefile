TOP = ../../..
include $(TOP)/config/make.cfg

INC = -I $(AROS_INCLUDES)

LIBS = -L$(LIBDIR) -lexec -larossupport -larosc -lgcc1 

all: aros.rom

entrycode.o : entrycode.s
	m68k-linux-as $^ -o $@

%.o : %.c
	m68k-linux-gcc -c $^ -o $@ $(CFLAGS)

aros_rom : entrycode.o palmgfx.o palm_reset.o
	m68k-linux-ld $^ -o $@ -e showsuccess -T linkerscript $(LIBS)

aros.rom : aros_rom
	m68k-linux-objcopy -O binary $^ $@ -v -S --pad-to=0x10d00000
	cp $@ /home/arospalm/.xcopilot/aros.rom

clean :
	rm -rf *.o aros.rom aros_rom *.s ~/.xcopilot/aros.rom



