#   Copyright © 1995-2019, The AROS Development Team. All rights reserved.
#   $Id$
#
#   Desc: Makefile for ilbmtoicon
#

-include $(TOP)/config/make.cfg

HOST_LIBPNG ?= -lpng
HOST_LIBPNG_INCLUDES ?=
HOST_LIBPNG_LIBEXTRA ?=

USER_CFLAGS := -Wall -Wunused -O2

HOST_CC	    ?= $(CC)
HOST_STRIP  ?= strip
ILBMTOICON  ?= ilbmtoicon
INFOINFO    ?= infoinfo
ECHO	    ?= echo

# linking of i386 on x86_64 doesn't work unless you make
# sure to have the i386 build tools for your distribution
# installed (including libz-dev:i386 and libpng-dev:i386
# or their equivalents).

EXTRALIBS1 := -lz

ifneq ($(HOST_LIBPNG_INCLUDES),)
    HOST_CFLAGS  += $(HOST_LIBPNG_INCLUDES)
endif
ifneq ($(HOST_LIBPNG_LIBEXTRA),)
    HOST_LDFLAGS += $(HOST_LIBPNG_LIBEXTRA)
else
    EXTRALIBS1 := $(HOST_LIBPNG) $(EXTRALIBS1)
endif

all : $(ILBMTOICON) $(INFOINFO)

$(ILBMTOICON) : ilbmtoicon.c
	@$(ECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) $< -o $@ $(HOST_LDFLAGS) $(EXTRALIBS1)
	@$(HOST_STRIP) $@

$(INFOINFO) : infoinfo.c
	@$(ECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) $< -o $@ $(HOST_LDFLAGS)
	@$(HOST_STRIP) $@

clean:
	@$(RM) -f $(ILBMTOICON)
	@$(RM) -f $(INFOINFO)
