#   Copyright (C) 2001 AROS - The Amiga Research OS
#   $Id$
#
#   Desc: Makefile to create the Hardfile creation tool createhdfile
#

USER_CFLAGS := -Wall -Wunused -O2
-include $(TOP)/config/make.cfg

HOST_CC		?= $(CC)
HOST_STRIP	?= strip
MECHO		?= echo
CREATEHDFILE	?= createhdfile
HOST_CFLAGS	:= $(HOST_CFLAGS) -I$(ADFINC)

all : $(CREATEHDFILE)

$(CREATEHDFILE) : createhdfile.c
	@$(MECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) $< -o $@ $(ADFLIB)
	@$(HOST_STRIP) $(CREATEHDFILE)

clean :
	@$(RM) $(CREATEHDFILE)

.PHONY : all clean
