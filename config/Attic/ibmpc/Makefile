# (C) 1997-1998 AROS -  The Amiga Research OS
# $Id: Makefile

# This is the main makefile for ibmpc project.

.EXPORT_ALL_VARIABLES:

TOPDIR	:=	$(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi)
TOP	=	$(TOPDIR)/../..

include $(TOP)/config/host.cfg

IPATH	=	$(TOPDIR)/include
INCLUDE =	$(TOP)/bin/$(ARCH)-$(CPU)/gen/include
OBJDIR	=	$(TOPDIR)/lib
CC	=	gcc -I$(IPATH) -I$(INCLUDE) -O0 -m486 -mno-fp-ret-in-387 -Wall
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
EXEC	=	$(OBJDIR)/exec.o
EXPANSION=	$(OBJDIR)/expansion.o
UTILITY =	$(OBJDIR)/utility.o
AROS	=	$(OBJDIR)/aros.o
BOOPSI	=	$(OBJDIR)/boopsi.o
MATHIEEESINGBAS= $(OBJDIR)/mathieeesingbas.o
AMIGALIB = $(TOP)/bin/$(ARCH)-$(CPU)/AROS/lib/libamiga.a

OBJECTS =	$(HEADER) $(CORE) $(EXEC) $(EXPANSION) $(UTILITY) $(AROS) \
    $(BOOPSI) $(MATHIEEESINGBAS) 

all:	$(IMAGE)
	@echo
	@echo Generating Image file
	@echo
	@$(BUILD) $(BOOTSECT) $(SETUP) $(KERNEL) >$(IMAGE)
	@echo

install :
	@echo Writting Image on floppy...
	@echo
	@dd if=$(IMAGE) of=/dev/fd0
	@echo
	@echo All done. You have AROS on floppy.
	@echo

$(IMAGE): $(BOOTSECT) $(SETUP) $(KERNEL) $(BUILD)

$(KERNEL): $(OBJECTS)
	@echo -n Linking kernel...
	@$(LD) -o $@ $(OBJECTS) $(AMIGALIB) -noinhibit-exec
	@objcopy -O binary $@
	@echo Done!

$(HEADER): $(TOPDIR)/boot/head.S $(TOPDIR)/boot/init.c $(TOPDIR)/boot/logo.c \
	   $(TOPDIR)/boot/text.c
	@echo -n Compiling kernel header...
	@$(MAKE) -s -C boot $(HEADER)
	@echo Done!

$(CORE):
	@$(MAKE) -s -C rom $(CORE)

$(EXEC):
	@$(MAKE) -s -C rom $(EXEC)

$(AROS):
	@$(MAKE) -s -C rom $(AROS)

$(EXPANSION):
	@$(MAKE) -s -C rom $(EXPANSION)

$(UTILITY):
	@$(MAKE) -s -C rom $(UTILITY)

$(BOOPSI):
	@$(MAKE) -s -C rom $(BOOPSI)

$(MATHIEEESINGBAS):
	@$(MAKE) -s -C rom $(MATHIEEESINGBAS)

$(BUILD): $(BUILD).c
	@echo -n Compiling build tool...
	@$(MAKE) -s -C boot/tools $(BUILD)
	@echo Done!

$(BOOTSECT): $(TOPDIR)/boot/bootsect.S
	@$(MAKE) -s -C boot $(BOOTSECT)

$(SETUP): $(TOPDIR)/boot/setup.S $(TOPDIR)/boot/video.S
	@$(MAKE) -s -C boot $(SETUP)

clean::
	@echo Cleaning up the mess...
	@$(MAKE) -s -C boot clean
	@$(MAKE) -s -C boot/tools clean
	@$(MAKE) -s -C rom clean
	@rm -f $(KERNEL) $(IMAGE) Output.map

force:: clean all

exec::
	@rm -f $(EXEC)
