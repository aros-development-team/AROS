#   Copyright © 1995-2001, The AROS Development Team. All rights reserved.
#   $Id$
#
#   Desc: Makefile to create the gensets
#

USER_CFLAGS := -Wall -Wunused -O2
-include $(TOP)/config/make.cfg

HOST_CC	    ?= $(CC)
HOST_STRIP  ?= strip
MECHO	    ?= echo
GENSETS	    ?= gensets
HOST_CFLAGS ?= $(USER_CFLAGS)

all : $(GENSETS)

$(GENSETS) : gensets.c
	@$(MECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) $< -o $@
	@$(HOST_STRIP) $(GENSETS)

clean :
	@$(RM) $(GENSETS)

.PHONY : all clean
