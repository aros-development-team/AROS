# (C) 1997-1998 AROS -  The Amiga Research OS
# $Id: Makefile

# This is the main makefile for ibmpc project.

.EXPORT_ALL_VARIABLES:

TOPDIR	:=	$(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi)

IPATH	=	$(TOPDIR)/include
OBJDIR	=	$(TOPDIR)/lib
CC	=	gcc -I$(IPATH) -O1 -m486 -Wall
LD	=	ld -Ttext 0x1000 -e startup_32 -N -Map Output.map
LINK	=	ld -N -r -s
MAKE	=	make

IMAGE	=	$(TOPDIR)/bin/Image

BUILD	=	$(TOPDIR)/boot/tools/build
BOOTSECT=	$(TOPDIR)/bin/bootsect
SETUP	=	$(TOPDIR)/bin/setup
KERNEL	=	$(TOPDIR)/bin/kernel
HEADER	=	$(OBJDIR)/header.o
CORE	=	$(OBJDIR)/core.o

OBJECTS	=	$(HEADER) $(CORE)

INCLUDE	=	~/AROS/bin/linux-i386/gen/include

all:	$(IMAGE)
	@echo
	@echo Generating Image file
	@echo
	@$(BUILD) $(BOOTSECT) $(SETUP) $(KERNEL) >$(IMAGE)
	@echo
	@echo Writting Image on floppy...
	@echo
	@dd if=$(IMAGE) of=/dev/fd0
	@echo
	@echo All done. You have AROS on floppy.
	@echo

$(IMAGE): $(BOOTSECT) $(SETUP) $(KERNEL) $(BUILD)

$(KERNEL): $(OBJECTS)
	@echo -n Linking kernel...
	@$(LD) -o $@ $(OBJECTS)
	@objcopy -O binary $@
	@echo OK

$(HEADER): $(TOPDIR)/boot/head.S $(TOPDIR)/boot/init.c $(TOPDIR)/boot/logo.c \
           $(TOPDIR)/boot/text.c
	@echo -n Compiling kernel header...
	@$(MAKE) -s -C boot $(HEADER)
	@echo OK

$(CORE):
	@$(MAKE) -s -C rom $(CORE)

$(BUILD): $(BUILD).c
	@echo -n Compiling build tool...
	@$(MAKE) -s -C boot/tools $(BUILD)
	@echo OK

$(BOOTSECT): $(TOPDIR)/boot/bootsect.S
	@$(MAKE) -s -C boot $(BOOTSECT)

$(SETUP): $(TOPDIR)/boot/setup.S $(TOPDIR)/boot/video.S
	@$(MAKE) -s -C boot $(SETUP)

clean:
	@echo Cleaning up the mess...
	@$(MAKE) -s -C boot clean
	@$(MAKE) -s -C boot/tools clean
	@$(MAKE) -s -C rom clean
	@rm -f $(KERNEL) $(IMAGE)