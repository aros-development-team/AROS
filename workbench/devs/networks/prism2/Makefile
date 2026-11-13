CFLAGS = -msmall-code -O2 -ansi -Wall -Wno-uninitialized -Wno-parentheses
CC = gcc $(CFLAGS)
MAKEFILE = Makefile
LINKER = ld -e _Main -s
AMIGA_LIB = -lamiga
C_LIB = -lnix/libnix13

DEV_NAME = prism2.device
OBJS = device.o unit.o encryption.o encryption_68k.o request.o pci.o\
 prometheus.o powerpci.o pccard.o timer.o
HDRS = device.h wireless.h io.h
OBJS += registration.o

all: $(DEV_NAME)

.c.o :
	$(CC) -c $<

$(DEV_NAME): $(MAKEFILE) $(OBJS)
	$(LINKER) $(OBJS) $(AMIGA_LIB) $(C_LIB) -o $(DEV_NAME)
	Protect "$(DEV_NAME)" "-e"

$(OBJS) startup.o: $(MAKEFILE)
$(OBJS) startup.o: $(HDRS)
unit.o: prism2.h
pci.o prometheus.o powerpci.o: pci.h

install: all
	Copy "$(DEV_NAME)" "DEVS:networks/" CLONE
	Flush "$(DEV_NAME)"
	Copy $(CMD_NAME) C: CLONE

clean:
	Delete "$(DEV_NAME)" "#?.o" $(CMD_NAME)

