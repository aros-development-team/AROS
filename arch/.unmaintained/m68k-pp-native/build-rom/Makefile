TOP = ../../..
include $(TOP)/config/make.cfg

INC = -I $(AROS_INCLUDES)

OBJS := $(OSGENDIR)/graphics/graphics_driver.o				\
	$(OSGENDIR)/graphics/objcache.o					\
	$(OSGENDIR)/graphics/dispinfo.o					\
	$(OSGENDIR)/graphics/fakegfxhidd.o

LIBS = -L$(LIBDIR)    \
	-lexec        \
	-laros        \
	-larossupport \
	-lbattclock   \
	-lboot        \
	-lconsole     \
	-ldos         \
	-lexpansion   \
	-lgameport    \
	-lgraphics    \
	-lhiddclass   \
	-lhiddgraphicsstubs \
	-linput       \
	-lintuition   \
	-lkeyboard    \
	-lkeymap      \
	-llayers      \
	-lmathffp     \
	-lmathieeesingbas \
	-lmisc        \
	-loop         \
	-ltimer       \
	-lutility     \
	-lworkbench   \
	-lamiga       \
	-larosc       \
	-lgcc1       \


all: aros.rom

entrycode.o : entrycode.s
	@$(ECHO) Assembling $^...
	@$(CC) -c $^

%.o : %.c
	@$(ECHO) Compiling $^...
	@$(CC) -c $^ -o $@ $(CFLAGS)

aros_rom : entrycode.o gfx.o reset.o
	@$(ECHO) Linking $^...
	$(CC) $^ $(OBJS) -o $@ -Wl,-T,linkerscript $(LIBS) -nostdlib

aros.rom : aros_rom
	@$(OBJCOPY) -O binary $^ $@ -v -S --pad-to=0x10d00000
	@$(RM) $(HOME)/.xcopilot/aros.rom
	@$(CP) $@ $(HOME)/.xcopilot/aros.rom

clean :
	$(RM) *.o aros.rom aros_rom *.s ~/.xcopilot/aros.rom
