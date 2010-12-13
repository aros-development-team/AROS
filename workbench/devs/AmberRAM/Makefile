# File: Makefile

CFLAGS = -msmall-code -DAMIGAOS -O3 -ansi -Wall -Wno-uninitialized\
	-Wno-parentheses
CC = gcc $(CFLAGS)
MAKEFILE = Makefile
LINKER = ld -e _Main -s

AMIGA_LIB = GG:lib/libamiga.a

HANDLER = amber-ram-handler
OBJS = resident.o handler.o commands.o filesystem.o notification.o support.o
HDRS = handler.h handler_protos.h
STARTUP = startup.o

.c.o :
	$(CC) -c $<

$(HANDLER): $(STARTUP) $(MAKEFILE) $(OBJS)
	$(LINKER) $(STARTUP) $(OBJS) $(AMIGA_LIB) -o $(HANDLER)
	Protect "$(HANDLER)" -e

$(OBJS) $(STARTUP): $(MAKEFILE) $(HDRS)

install: $(HANDLER)
	Copy CLONE "$(HANDLER)" "L:"

test: $(HANDLER)
	Execute "Dismount" "RAM1:"
	Mount "RAM1"

clean:
	Delete "$(HANDLER)" "#?.o"


