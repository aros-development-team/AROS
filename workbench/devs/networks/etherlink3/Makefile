CFLAGS = -msmall-code -O0 -ansi -Wall -Wno-uninitialized -Wno-parentheses
CC = gcc $(CFLAGS)
#CC = gcc
MAKEFILE = Makefile
LINKER = ld -e _Main -s
AMIGA_LIB = -lamiga
C_LIB = -lnix/libnix13

DEV_NAME = etherlink3.device
OBJS = device.o unit.o request.o pci.o prometheus.o powerpci.o pccard.o
#OBJS = device.o unit.o request.o pci.o
HDRS = device.h endian.h io.h

all: $(DEV_NAME)

.c.o :
	$(CC) -c $<

$(DEV_NAME): $(MAKEFILE) $(OBJS)
	$(LINKER) $(OBJS) $(AMIGA_LIB) $(C_LIB) -o $(DEV_NAME)
	Protect "$(DEV_NAME)" "-e"

$(OBJS) startup.o: $(MAKEFILE) $(HDRS)
$(OBJS): $(HDRS)
unit.o: etherlink3.h mii.h
pci.o prometheus.o powerpci.o: pci.h
pccard.o: io.h

install: all
	Copy "$(DEV_NAME)" "DEVS:networks/" CLONE
	Flush "$(DEV_NAME)"

clean:
	Delete "$(DEV_NAME)" "#?.o"

