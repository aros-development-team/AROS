#   Copyright (C) 2002 AROS - The Amiga Research OS
#   $Id$
#
#   Desc: Makefile to create genmodule
#

USER_CFLAGS := -Wall -Wunused -O2
-include $(TOP)/config/make.cfg

HOST_CC	    ?= gcc
HOST_STRIP  ?= strip
MECHO	    ?= echo
HOST_CFLAGS ?= $(USER_CFLAGS)
GENMODULE   ?= genmodule

all : $(GENMODULE)

SRCS := genmodule.c \
        readconfig.c \
	readref.c \
	writeincclib.c \
	writeincproto.c \
	writeincdefines.c \
	writeinclibdefs.c \
	writestart.c \
	writeend.c
	
INCS := genmodule.h

$(GENMODULE) : $(SRCS) $(INCS)
	@$(MECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) $(SRCS) -o $@
	@$(HOST_STRIP) $@


clean :
	@$(RM) $(COLLECT-AROS)

.PHONY : all clean
