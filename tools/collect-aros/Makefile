#   Copyright (C) 2000 AROS - The Amiga Research OS
#   $Id$
#
#   Desc: Makefile to create the gensets
#

USER_CFLAGS := -Wall -Wunused -O2
-include $(TOP)/config/make.cfg

HOST_CC	    ?= $(CC)
HOST_STRIP  ?= strip
MECHO	    ?= echo
GENSETS	    ?= gensets
HOST_CFLAGS ?= $(USER_CFLAGS)

all : $(COLLECT-AROS) $(TOOLDIR)/specs

$(COLLECT-AROS) : collect-aros.c gensets.c
	@$(MECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) $^ -o $@
	@$(HOST_STRIP) $(COLLECT-AROS)

$(TOOLDIR)/specs:
	@echo -e "*linker:\n$(COLLECT-AROS)" > $(TOOLDIR)/specs
clean :
	@$(RM) $(COLLECT-AROS)

.PHONY : all clean
