#   Copyright © 2019, The AROS Development Team. All rights reserved.
#   $Id$
#
#   Desc: Makefile to create mkkeymap
#

USER_CFLAGS := -Wall -Werror -Wunused -O2

-include $(TOP)/config/make.cfg
include Makefile.deps

HOST_CC	       ?= gcc
HOST_STRIP     ?= strip
MECHO	       ?= echo
HOST_CFLAGS    ?= $(USER_CFLAGS)
MKKEYMAP      ?= mkkeymap

all : $(MKKEYMAP)

$(MKKEYMAP) : $(MKKEYMAP_DEPS)
	@$(MECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) -g $(HOST_CFLAGS) -I$(GENINCDIR) -I$(TOP)/$(CURDIR) $(MKKEYMAP_SRCS) -o $@
#	@$(HOST_CC) -E $(HOST_CFLAGS) -I$(GENINCDIR) -I$(TOP)/$(CURDIR) $(MKKEYMAP_SRCS) -o $@.C
#	@$(HOST_STRIP) $@

clean :
	@$(RM) $(MKKEYMAP)

.PHONY : all clean
