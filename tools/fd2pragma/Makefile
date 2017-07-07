# Copyright © 2003-2017, The AROS Development Team. All rights reserved.
# $Id$

include $(TOP)/config/make.cfg

NOWARN_FLAGS := -Wno-format-security -Wno-empty-body

all : $(FD2PRAGMA)

$(FD2PRAGMA) : fd2pragma.c
	@$(ECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) $(USER_CFLAGS) $(NOWARN_FLAGS) $< -o $@

clean:
	@$(RM) -f $(FD2PRAGMA)
