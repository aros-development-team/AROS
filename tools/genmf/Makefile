#   Copyright © 2000, The AROS Development Team. All rights reserved.
#   $Id$
#
#   Desc: Makefile for genmf
#

# Things of interest.
SRC	:=  genmf.c

-include $(TOP)/config/make.cfg

HOST_CC	    ?= $(CC)
HOST_STRIP  ?= strip
HOST_MKDIR  ?= mkdir
GENMF	    ?= genmf
HOST_CFLAGS ?= -Wall -g -O

all : $(GENMF)

$(GENMF) : genmf.c
	@$(MECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) $< -o $(GENMF)

clean :
	@$(RM) $(GENMF)

check :
	$(GENMF) tmpl.test < in.test
	$(GENMF) --curdir=`pwd` tmpl.test < in.test

test : check
