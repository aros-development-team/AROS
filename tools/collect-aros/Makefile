# Copyright © 2000-2008, The AROS Development Team. All rights reserved.
# $Id$
#
# Makefile to build collect-aros.

USER_CFLAGS += -Wall -Wunused -O2
-include $(TOP)/config/make.cfg

HOST_CC	     ?= $(CC)
HOST_STRIP   ?= strip
MECHO	     ?= echo
COLLECT-AROS ?= collect-aros
RM           ?= rm
COLLECT-AROS-BACKEND   ?= backend-generic.c
COLLECT-AROS-DOCOMMAND ?= docommand-exec.c
ifneq ($(SRCDIR),)
COLLECT-AROS-DIR := $(TOP)/tools/collect-aros
else
COLLECT-AROS-DIR := .
endif
COLLECT_CFLAGS  ?= $(USER_CFLAGS) -I $(COLLECT-AROS-DIR)

all : $(COLLECT-AROS)

COLLECT-AROS-FILES := collect-aros.c gensets.c misc.c $(COLLECT-AROS-BACKEND) \
                      $(COLLECT-AROS-DOCOMMAND)
$(COLLECT-AROS) : misc.h ldscript.h gensets.h docommand.h backend.h $(COLLECT-AROS-DIR)/env.h
$(COLLECT-AROS) : $(COLLECT-AROS-FILES)
	@$(MECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) -o $@ $(COLLECT_CFLAGS) $(COLLECT-AROS-FILES) $(HOST_LDFLAGS) 


clean :
	@$(RM) $(COLLECT-AROS)

.PHONY : all clean
