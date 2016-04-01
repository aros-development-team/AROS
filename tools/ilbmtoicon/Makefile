#   Copyright © 1995-2012, The AROS Development Team. All rights reserved.
#   $Id$
#
#   Desc: Makefile for ilbmtoicon
#

-include $(TOP)/config/make.cfg

LIBPNG ?= png
LIBPNG_INCLUDES ?=
LIBPNG_LIB ?= 

USER_CFLAGS := -Wall -Wunused -O2

HOST_CC	    ?= $(CC)
HOST_STRIP  ?= strip
ILBMTOICON  ?= ilbmtoicon
INFOINFO    ?= infoinfo  
MECHO	    ?= echo

ifneq ($(LIBPNG_INCLUDES),)
    HOST_CFLAGS  += $(LIBPNG_INCLUDES)
endif
ifneq ($(LIBPNG_LIB),)
    HOST_LDFLAGS += $(LIBPNG_LIB)
endif

# linking of i386 on x86_64 doesn't work unless you make
# sure to have the i386 build tools for your distribution
# installed (including libz-dev:i386 and libpng-dev:i386
# or their equivalents).
EXTRALIBS1 := -l$(LIBPNG) -lz

all : $(ILBMTOICON) $(INFOINFO)

$(ILBMTOICON) : ilbmtoicon.c
	@$(ECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) $(HOST_LDFLAGS) $< -o $@ $(EXTRALIBS1)
	@$(HOST_STRIP) $@

$(INFOINFO) : infoinfo.c
	@$(ECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) $(HOST_LDFLAGS) $< -o $@
	@$(HOST_STRIP) $@

clean:
	@$(RM) -f $(ILBMTOICON)
	@$(RM) -f $(INFOINFO)
