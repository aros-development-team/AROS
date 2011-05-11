#   Copyright © 2011, The AROS Development Team. All rights reserved.
#   $Id$
#
#   Desc: Makefile for setrev
#

SRC	:= setrev.c

include $(TOP)/config/make.cfg

all : $(SETREV)

$(SETREV) : $(SRC)
	@$(MECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) $(HOST_LDFLAGS) $< -o $@
	@$(HOST_STRIP) $@


clean:
	@$(RM) -f $(SETREV)
