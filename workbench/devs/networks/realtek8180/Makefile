CFLAGS = -msmall-code -O2 -ansi -Wall -Wno-uninitialized -Wno-parentheses
CC = gcc $(CFLAGS)
MAKEFILE = Makefile
LINKER = ld -e _Main -s
AMIGA_LIB = -lamiga
C_LIB = -lnix/libnix13

DEV_NAME = realtek8180.device
OBJS = device.o unit.o crypto.o request.o pci.o prometheus.o powerpci.o
HDRS = device.h wireless.h io.h

all: $(DEV_NAME) $(CMD_NAME)

.c.o :
	$(CC) -c $<

$(DEV_NAME): $(MAKEFILE) $(OBJS)
	$(LINKER) $(OBJS) $(AMIGA_LIB) $(C_LIB) -o $(DEV_NAME)
	Protect "$(DEV_NAME)" "-e"

$(OBJS) $(CMD_NAME).o startup.o: $(MAKEFILE)
$(OBJS) startup.o: $(HDRS)
$(CMD_NAME).o: wireless.h
unit.o radio.o eeprom: realtek8187.h
pci.o prometheus.o powerpci.o: pci.h

install: all
	Copy "$(DEV_NAME)" "DEVS:networks/" CLONE
	Flush "$(DEV_NAME)"
	Copy $(CMD_NAME) C: CLONE

clean:
	Delete "$(DEV_NAME)" "#?.o"

