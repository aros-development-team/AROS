#   Copyright © 2011, The AROS Development Team. All rights reserved.
#   $Id$
#
#   Desc: Makefile for elf2hunk
#

# Things of interest.
SRC	:=  elf2hunk.c

-include $(TOP)/config/make.cfg

HOST_CC	    ?= $(CC)
HOST_STRIP  ?= strip
HOST_MKDIR  ?= mkdir
ELF2HUNK    ?= elf2hunk
HOST_CFLAGS ?= -Wall -g -O

all : $(ELF2HUNK)

$(ELF2HUNK) : $(SRC)
	@$(MECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) $< -o $(ELF2HUNK)

clean :
	@$(RM) $(ELF2HUNK)

check :
	true

test : check
