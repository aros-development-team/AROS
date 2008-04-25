# Copyright © 2003, The AROS Development Team. All rights reserved.
# $Id$

include $(TOP)/config/make.cfg

USER_CFLAGS := -Wall -Wstrict-prototypes '-DVERSION="1.37"' -g -O2

all : $(FD2INLINE)

$(FD2INLINE) : fd2inline.c
	@$(ECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) $(USER_CFLAGS) $< -o $@

clean:
	@$(RM) -f $(FD2INLINE)
