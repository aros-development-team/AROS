#   Copyright (C) 2001 AROS - The Amiga Research OS
#   $Id$
#
#   Desc: Makefile to create the mkfsaffs filesystem generator
#

USER_CFLAGS := -Wall -Wunused -O2
-include $(TOP)/config/make.cfg

HOST_CC		?= $(CC)
HOST_STRIP	?= strip
MECHO		?= echo
CREATEHDFILE	?= mkfsaffs
HOST_CFLAGS	:= $(HOST_CFLAGS) -I$(ADFINC)

all : $(CREATEHDFILE)

$(CREATEHDFILE) : mkfsaffs.c
	@$(MECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) $< -o $@ $(ADFLIB)
	@$(HOST_STRIP) $(MKFSAFFS)

clean :
	@$(RM) $(MKFSAFFS)

.PHONY : all clean
