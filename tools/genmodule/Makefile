#   Copyright (C) 2002 AROS - The Amiga Research OS
#   $Id$
#
#   Desc: Makefile to create genmodule
#

USER_CFLAGS := -Wall -Wunused -O2

-include $(TOP)/config/make.cfg
include Makefile.deps

HOST_CC	       ?= gcc
HOST_STRIP     ?= strip
MECHO	       ?= echo
HOST_CFLAGS    ?= $(USER_CFLAGS)
GENMODULE      ?= genmodule
GENLIBDEFS     ?= genlibdefs
GENMODULEDUMMY ?= genmoduledummy

all : $(GENMODULE) $(GENLIBDEFS) $(GENMODULEDUMMY)

GENMODULEINCLUDE := $(dir $(GENMODULE))/genmod_inc/aros/libcall.h

$(GENMODULE) : $(GENMODULE_DEPS) $(GENMODULEINCLUDE)
	@$(MECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) $(GENMODULE_SRCS) -o $@
	@$(HOST_STRIP) $@

$(GENLIBDEFS) : $(GENLIBDEFS_DEPS)
	@$(MECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) $(GENLIBDEFS_SRCS) -o $@
	@$(HOST_STRIP) $@

$(GENMODULEDUMMY) : $(GENMODULEDUMMY_DEPS)
	@$(MECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) $(GENMODULEDUMMY_SRCS) -o $@
	@$(HOST_STRIP) $@

$(GENMODULEINCLUDE) : include/aros/libcall.h
	@$(MKDIR) $(dir $@)
	@$(CP) $< $@

clean :
	@$(RM) $(GENMODULE) $(GENLIBDEFS) $(GENMODULEDUMMY) $(GENMODULEINCLUDE)

.PHONY : all clean
