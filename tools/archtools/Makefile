# Copyright © 2000-2003, The AROS Development Team. All rights reserved.
# $Id$

-include $(TOP)/config/make.cfg

USER_CFLAGS := -Wall -Wunused -O0 -g

HOST_CC	    ?= $(CC)
HOST_STRIP  ?= strip
ARCHTOOL    ?= archtool
MECHO	    ?= echo

all : $(ARCHTOOL)

$(ARCHTOOL) : archtool.c
	@$(MECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) $(USER_CFLAGS) $< -o $@

#	@$(HOST_STRIP) $@

clean:
	@$(RM) -f $(ARCHTOOL)
