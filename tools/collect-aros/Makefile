#   Copyright (C) 2000 AROS - The Amiga Research OS
#   $Id$
#
#   Desc: Makefile to create the gensets
#

USER_CFLAGS := -Wall -Wunused -O2
-include $(TOP)/config/make.cfg

HOST_CC	    ?= $(CC)
HOST_STRIP  ?= strip
MECHO	    ?= echo
GENSETS	    ?= gensets
HOST_CFLAGS ?= $(USER_CFLAGS)

all : collect-aros.h $(COLLECT-AROS)

$(COLLECT-AROS) : collect-aros.c gensets.c
	@$(MECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) $^ -o $@
	@$(HOST_STRIP) $(COLLECT-AROS)

collect-aros.h :
	@echo "#define COLLECT2PATH \"$(shell $(TARGET_CC) -print-prog-name=collect2)\"" > collect-aros.h
	@echo "#define NMPATH \"$(NM_PLAIN)\"" >> collect-aros.h
	@echo "#define GCCPATH \"$(TARGET_CC)\"" >> collect-aros.h
	@echo "#define MVPATH \"$(MV)\"" >> collect-aros.h

clean :
	@$(RM) $(COLLECT-AROS) collect-aros.h

.PHONY : all clean
