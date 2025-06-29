# Copyright © 2000-2025 The AROS Development Team. All rights reserved.
# $Id$
#
# Makefile to build collect-aros.

USER_CFLAGS += -Wall -Wunused -O2
-include $(TOP)/config/make.cfg
Q=@

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
ifneq ($(AROS_LIB),)
AROS_LIB_DIR := $(AROS_LIB)
else
AROS_LIB_DIR := .
endif
ifneq ($(CAGENDIR),)
COLLECTAROSINCLUDES := -I $(CAGENDIR)
endif
COLLECT_CFLAGS  ?= $(HOST_CFLAGS) $(USER_CFLAGS) $(COLLECTAROSINCLUDES) -I $(COLLECT-AROS-DIR) -DOBJLIBDIR=\"$(AROS_LIB_DIR)\"
COLLECTAROSBIN ?= $(COLLECT-AROS)

ifeq ($(AROS_HOST_ARCH),aros)
COLLECT_CFLAGS += -D_HAVE_LIBIBERTY_
HOST_LDFLAGS += -lbfd -liberty -lz
COLLECT-AROS-BACKEND   := backend-bfd.c
endif
ifeq ($(AROS_HOST_ARCH),mingw32)
COLLECT_CFLAGS += -D_HAVE_LIBIBERTY_
HOST_LDFLAGS += -liberty
COLLECT-AROS-DOCOMMAND := docommand-spawn.c
endif

all : $(COLLECTAROSBIN)

COLLECT-AROS-FILES := collect-aros.c gensets.c misc.c $(COLLECT-AROS-BACKEND) \
                      $(COLLECT-AROS-DOCOMMAND)
$(COLLECTAROSBIN) : misc.h ldscript.h gensets.h docommand.h backend.h $(CAGENDIR)/env.h
$(COLLECTAROSBIN) : $(COLLECT-AROS-FILES)
	$(Q)$(MECHO) "Compiling $(notdir $@)..."
	$(Q)$(HOST_CC) -o $@ $(COLLECT_CFLAGS) $(COLLECT-AROS-FILES) $(HOST_LDFLAGS) 


clean :
	$(Q)$(RM) $(COLLECTAROSBIN)

.PHONY : all clean
