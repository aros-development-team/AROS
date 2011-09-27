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
INFOINFO    ?= infoinfo  
MECHO	    ?= echo

all : $(ILBMTOICON) $(INFOINFO)

$(ILBMTOICON) : ilbmtoicon.c
	@$(MECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) $(HOST_LDFLAGS) $< -o $@ -lpng
	@$(HOST_STRIP) $@

$(INFOINFO) : infoinfo.c
	@$(MECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) $(HOST_LDFLAGS) $< -o $@ -lpng
	@$(HOST_STRIP) $@

clean:
	@$(RM) -f $(ILBMTOICON)
