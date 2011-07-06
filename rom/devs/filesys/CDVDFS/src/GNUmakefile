#
# Makefile for AmiCDROM with GNUC
#

#Turn these flags on to enable debugging
#DEBUGLIBS  = -ldebug
#DEBUGFLAGS = -DDEBUG

#And these ones are for normal operation
DEBUGLIBS  =
DEBUGFLAGS = -DNDEBUG

# some shortcuts

RM		= /c/delete quiet
NOEXEC		= /c/protect flags=-e
STRIP		= strip --remove-section=.comment
CC		= gcc

OPT		=-O2

# -fbaserel does not work, -msmall-code likewise, -fno-builtin requires ixemul

CFLAGS		= $(OPT) -fno-function-cse

LDFLAGS1	= -noixemul -nostartfiles
LDFLAGS2	= -noixemul

OS=$(shell uname)
ifeq "$(OS)" "MorphOS"
LDLIBS1 =
KILLDEV = KillDevice RemCDFS
else
LDLIBS1 = -lamiga
KILLDEV =
endif

LDLIBS2		=

.c.o:
		$(CC) $(CFLAGS) $(DEBUGFLAGS) -o $@ $< -c

# all object files we have

OBJS1		= device.o volumes.o intui.o devsupp.o cdrom.o iso9660.o \
		  rock.o joliet.o hfs.o generic.o path.o charset.o prefs.o
OBJS2		= main.o cdrom.o iso9660.o rock.o joliet.o hfs.o generic.o path.o charset.o
OBJS3		= cdcontrol.o
OBJS4		= checkcd.o cdrom.o iso9660.o rock.o joliet.o hfs.o generic.o path.o charset.o

# general targets

all:		cdrom-handler cdrom cdcontrol checkcd $(KILLDEV)

cdrom-handler:	$(OBJS1)
		$(CC) $(LDFLAGS1) -o $@ $(OBJS1) $(LDLIBS1) $(DEBUGLIBS)
		$(STRIP) $@
		$(NOEXEC) $@

cdrom:		$(OBJS2)
		$(CC) $(LDFLAGS2) -o $@ $(OBJS2) $(LDLIBS2) $(DEBUGLIBS)
		$(STRIP) $@

cdcontrol:	$(OBJS3)
		$(CC) $(LDFLAGS2) -o $@ $(OBJS3) $(LDLIBS2) $(DEBUGLIBS)
		$(STRIP) $@

checkcd:	$(OBJS4)
		$(CC) $(LDFLAGS2) -o $@ $(OBJS4) $(LDLIBS2) $(DEBUGLIBS)
		$(STRIP) $@

KillDevice:	KillDevice.c
		$(CC) $(CFLAGS) $(LDFLAGS1) -o $@ KillDevice.c
		$(STRIP) $@

RemCDFS: remcdfs.c
		$(CC) $(CFLAGS) $(LDFLAGS1) -o $@ $<
		$(STRIP) $@

# dependencies

iso9660.o:	iso9660.c cdrom.h iso9660.h rock.h generic.h
cdrom.o:	cdrom.c cdrom.h
rock.o:		rock.c cdrom.h iso9660.h rock.h generic.h
joliet.o:	joliet.h iso9660.h
device.o:	device.c cdrom.h device.h intui.h devsupp.h generic.h \
		cdcontrol.h rock.h aros_stuff.h
devsupp.o:	devsupp.c devsupp.h cdrom.h intui.h device.h generic.h params.h
intui.o:	intui.c intui.h
generic.o:	generic.c generic.h cdrom.h iso9660.h rock.h joliet.h params.h
main.o:		main.c cdrom.h iso9660.h rock.h generic.h hfs.h
hfs.o:		hfs.c hfs.h generic.h cdrom.h params.h
cdcontrol.o:	cdcontrol.c cdcontrol.h
checkcd.o:	checkcd.c iso9660.h generic.h cdrom.h rock.h
volimes.o:	volumes.c volumes.h cdrom.h device.h devsupp.h generic.g debug.h \
		path.h globals.h aros_stuff.h clib_stuff.h

# install

install:        all
		Copy cdrom-handler L:
		Copy cdcontrol cdrom checkcd $(KILLDEV) C:

# prepare a distribution

dist:           all
		Copy cdrom-handler /L
		Copy cdcontrol cdrom checkcd $(KILLDEV) /C


# delete all nifty stuff

clean:
		-$(RM) *.o cdrom cdrom-handler cdcontrol checkcd KillDevice RemCDFS
