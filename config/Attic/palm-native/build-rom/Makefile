TOP = ../../..
include $(TOP)/config/make.cfg

INC = -I $(AROS_INCLUDES)

LIBS = -L$(LIBDIR) -lexec -larossupport -larosc -lgcc1 

all: aros.rom

entrycode.o : entrycode.s
	@$(ECHO) Assembling $^...
	@$(CC) -c $^

%.o : %.c
	@$(ECHO) Compiling $^...
	@$(CC) -c $^ -o $@ $(CFLAGS)

aros_rom : entrycode.o palmgfx.o palm_reset.o
	@$(ECHO) Linking $^...
	@$(CC) $^ -o $@ -Wl,-e,showsuccess,-T,linkerscript $(LIBS) -nostdlib

aros.rom : aros_rom
	@$(OBJCOPY) -O binary $^ $@ -v -S --pad-to=0x10d00000
	@$(CP) $@ $(HOME)/.xcopilot/aros.rom

clean :
	$(RM) *.o aros.rom aros_rom *.s ~/.xcopilot/aros.rom



