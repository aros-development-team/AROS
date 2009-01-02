#
# Makefile for ezSysLog
#

INSTALL_DEST := /SYS/MOSNet

CC     = gcc
CFLAGS = -W -Wall -noixemul -I ../include -O2 -Wpointer-arith -Wno-parentheses
STRIP = strip --remove-section .comment

#########################################################################

OBJS = main.o hooks.o colorlist.o str.o

#########################################################################

.DEFAULT:
	@echo "Nothing to do."

.PHONY: all clean

all: MNSysLog

clean:
	rm -f *.o MNSysLog MNSysLog.db

install: MNSysLog
	cp MNSysLog MNSysLog.info $(INSTALL_DEST)

package: MNSysLog
	cp MNSysLog MNSysLog.info $(PKG_DEST)

MNSysLog.db: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ -ldebug

MNSysLog: MNSysLog.db
	$(STRIP) $< -o $@
	protect $@ +e

main.o: main.c main.h colorlist.h

hooks.o: hooks.c main.h

colorlist.o: colorlist.c main.h str.h

str.o: str.c str.h

