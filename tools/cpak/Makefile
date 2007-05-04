#   Copyright © 2000, The AROS Development Team. All rights reserved.
#   $Id$
#
#   Desc: Makefile to create the (soon to be obsolete) cpak
#

USER_CFLAGS := -Wall -Wunused -O2
-include $(TOP)/config/make.cfg

HOST_CC	    ?= $(CC)
HOST_STRIP  ?= strip
MECHO	    ?= echo
CPAK	    ?= cpak
HOST_CFLAGS ?= $(USER_CFLAGS)

all : $(CPAK)

$(CPAK) : cpak.c
	@$(MECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) $< -o $@
	@$(HOST_STRIP) $(CPAK)

clean :
	@$(RM) $(CPAK)

.PHONY : all clean
