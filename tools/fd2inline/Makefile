# Copyright © 2003-2022, The AROS Development Team. All rights reserved.
# $Id$

include $(TOP)/config/make.cfg

USER_CPPFLAGS := '-DVERSION="1.39"'
USER_CFLAGS := -Wall -Wstrict-prototypes -std=c11 -g -O2

all : $(FD2INLINE)

$(FD2INLINE) : fd2inline.c
	@$(ECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) $(USER_CPPFLAGS) $(USER_CFLAGS) $< -o $@

clean:
	@$(RM) -f $(FD2INLINE)
