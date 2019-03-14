#   Copyright © 2019, The AROS Development Team. All rights reserved.
#   $Id$

USER_CFLAGS := -Wall -Werror -Wunused -O2
USER_CPPFLAGS :=

-include $(TOP)/config/make.cfg
include Makefile.deps

HOST_CC	       ?= gcc
HOST_STRIP     ?= strip
MECHO	       ?= echo
HOST_CFLAGS    ?= $(USER_CFLAGS)
HOST_CPPFLAGS    ?= $(USER_CPPFLAGS)
PARSEOFFSETS      ?= parseoffsets

all : $(PARSEOFFSETS)

$(PARSEOFFSETS) : $(PARSEOFFSETS_DEPS)
	@$(MECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) -g $(HOST_CFLAGS) $(HOST_CPPFLAGS) -fsanitize=undefined -I$(GENINCDIR) -I$(TOP)/$(CURDIR) $(PARSEOFFSETS_SRCS) -o $@
#	@$(HOST_STRIP) $@

clean :
	@$(RM) $(PARSEOFFSETS)

.PHONY : all clean
