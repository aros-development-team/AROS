#   Copyright © 1995-2011, The AROS Development Team. All rights reserved.
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

# Use libpng from MacPorts on MacOS X.
# We don't add these includes in configure script because doing so
# influences makecountry which uses libiconv. MacPorts libiconv has different ABI
# from system's one, this causes conflicts.
ifeq ($(AROS_HOST_ARCH),darwin)
    HOST_CFLAGS  += -isystem /opt/local/include
    HOST_LDFLAGS += -L/opt/local/lib/
endif

ifeq ($(AROS_HOST_ARCH),mingw32)
    EXTRALIBS2 := -lws2_32
endif
# linking of i386 on x86_64 doesn't work unless you make
# sure to have the i386 build tools for your distribution
# installed (including libz-dev:i386 and libpng-dev:i386
# or their equivalents).
EXTRALIBS1 := -lpng -lz

all : $(ILBMTOICON) $(INFOINFO)

$(ILBMTOICON) : ilbmtoicon.c
	@$(ECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) $(HOST_LDFLAGS) $< -o $@ $(EXTRALIBS1)
	@$(HOST_STRIP) $@

$(INFOINFO) : infoinfo.c
	@$(ECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) $(HOST_LDFLAGS) $< -o $@ $(EXTRALIBS2)
	@$(HOST_STRIP) $@

clean:
	@$(RM) -f $(ILBMTOICON)
