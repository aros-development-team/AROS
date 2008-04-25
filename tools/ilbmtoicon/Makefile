#   Copyright © 1995-2001, The AROS Development Team. All rights reserved.
#   $Id$
#
#   Desc: Makefile for ilbmtoicon
#

-include $(TOP)/config/make.cfg

USER_CFLAGS := -Wall -Wunused -O2

HOST_CC	    ?= $(CC)
HOST_STRIP  ?= strip
ILBMTOICON  ?= ilbmtoicon
MECHO	    ?= echo

all : $(ILBMTOICON)

$(ILBMTOICON) : ilbmtoicon.c
	@$(MECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) $< -o $@
	@$(HOST_STRIP) $@

clean:
	@$(RM) -f $(ILBMTOICON)
