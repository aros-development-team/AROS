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
COLLECT-AROS-BACKEND   ?= backend-generic.c
COLLECT-AROS-DOCOMMAND ?= docommand-exec.c

all : $(COLLECT-AROS)

COLLECT-AROS-FILES := collect-aros.c gensets.c misc.c $(COLLECT-AROS-BACKEND) \
                      $(COLLECT-AROS-DOCOMMAND)
$(COLLECT-AROS) : misc.h ldscript.h gensets.h docommand.h backend.h
$(COLLECT-AROS) : $(COLLECT-AROS-FILES)
	@$(MECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) $(COLLECT-AROS-FILES) -o $@


clean :
	@$(RM) $(COLLECT-AROS)

.PHONY : all clean
