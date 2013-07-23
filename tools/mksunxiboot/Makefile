#   Copyright © 2013, The AROS Development Team. All rights reserved.
#   $Id$
#
#   Desc: Makefile for mksunxiboot
#

-include $(TOP)/config/make.cfg

USER_CFLAGS := -Wall -Wunused -O2

HOST_CC	    ?= $(CC)
HOST_STRIP  ?= strip
MKSUNXIBOOT ?= mksunxiboot
MECHO	    ?= echo

all : $(MKSUNXIBOOT)

$(MKSUNXIBOOT) : mksunxiboot.c
	@$(MECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) $(USER_CFLAGS) $< -o $@
	@$(HOST_STRIP) $@

clean:
	@$(RM) -f $(MKSUNXIBOOT)
