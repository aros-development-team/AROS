# Copyright © 2000-2003, The AROS Development Team. All rights reserved.
# $Id$
#
# Makefile to build collect-aros.

USER_CFLAGS := -Wall -Wunused -O2
-include $(TOP)/config/make.cfg

HOST_CC	     ?= $(CC)
HOST_STRIP   ?= strip
MECHO	     ?= echo
HOST_CFLAGS  ?= $(USER_CFLAGS)
COLLECT-AROS ?= collect-aros
RM           ?= rm
MV           ?= mv

all : $(COLLECT-AROS)

$(COLLECT-AROS) : collect-aros.c gensets.c ldscript.h 
	@$(MECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) collect-aros.c gensets.c -o $@  -DMVPATH="\"$(MV)\""
	@$(HOST_STRIP) $(COLLECT-AROS)


clean :
	@$(RM) $(COLLECT-AROS)

.PHONY : all clean
