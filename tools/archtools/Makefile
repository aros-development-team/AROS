#   Copyright (C) 2000 AROS - The Amiga Research OS
#   $Id$
#
#   Desc: Makefile for archtool
#

-include $(TOP)/config/make.cfg

USER_CFLAGS := -Wall -Wunused -O2

HOST_CC	    ?= $(CC)
HOST_STRIP  ?= strip
ARCHTOOL    ?= archtool
MECHO	    ?= echo

all : $(ARCHTOOL)

$(ARCHTOOL) : archtool.c
	@$(MECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) $< -o $@
	@$(HOST_STRIP) $@

clean:
	@$(RM) -f $(ARCHTOOL)
