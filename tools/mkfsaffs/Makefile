#   Copyright © 2001, The AROS Development Team. All rights reserved.
#   $Id$
#
#   Desc: Makefile to create the mkfsaffs filesystem generator
#

USER_CFLAGS := -Wall -Wunused -O2
-include $(TOP)/config/make.cfg

HOST_CC		?= $(CC)
HOST_STRIP	?= strip
MECHO		?= echo
MKFSAFFS	?= mkfsaffs
HOST_CFLAGS	:= $(HOST_CFLAGS) -I$(ADFINC)

all : $(MKFSAFFS)

$(MKFSAFFS) : mkfsaffs.c
	@$(MECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) $< -o $@ -L$(dir $(ADFLIB)) -ladf
	@$(HOST_STRIP) $(MKFSAFFS)

clean :
	@$(RM) $(MKFSAFFS)

.PHONY : all clean
