# Copyright © 2003, The AROS Development Team. All rights reserved.
# $Id$

include $(AROS_SRC)/config/make.cfg

all : $(FD2PRAGMA)

$(FD2PRAGMA) : fd2pragma.c
	@$(ECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) $(USER_CFLAGS) $< -o $@

clean:
	@$(RM) -f $(FD2PRAGMA)
