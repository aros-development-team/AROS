#   Copyright (C) 2000 AROS - The Amiga Replacement OS
#   $Id$
#
#   Desc: Makefile for genmf
#

# Things of interest.
SRC	    :=  moddefs module includes

-include $(TOP)/config/make.cfg

HOST_AR	    ?= $(AR) cru
HOST_CC	    ?= $(CC)
HOST_STRIP  ?= strip
MKDIR	    ?= mkdir
MECHO	    ?= echo
TOOLDIR	    ?= .
MODTOOL	    ?= modtool
EXTADOC	    ?= extadoc
HOST_CFLAGS ?= -L../toollib

OBJDIR	    := $(TOOLDIR)/obj
HOST_CFLAGS := $(HOST_CFLAGS) -I.. -Wall -g -O -L$(OBJDIR) -L$(TOOLDIR)
MODLIB	    := $(OBJDIR)/libmodule.a

all : $(OBJDIR) $(MODLIB) $(MODTOOL)

$(MODLIB) : $(foreach f,$(SRC),$(OBJDIR)/$(f).o)
	@$(MECHO) "Building modtool/$(notdir $@)..."
	@$(HOST_AR) $@ $?

$(OBJDIR)/%.o : %.c
	@$(MECHO) "Compiling modtool/$(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) $< -c -o $@

$(MODTOOL) : modtool.c $(MODLIB)
	@$(MECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) modtool.c -lmodule -ltool -o $@

$(OBJDIR) :
	$(MKDIR) $@

clean :
	@$(RM) -r $(OBJDIR) $(MODLIB) $(MODTOOL) moddefs.h moddefs.h.bak
	@$(RM) -r clib proto defines inclines

