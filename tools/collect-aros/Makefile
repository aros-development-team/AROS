#   Copyright (C) 2000 AROS - The Amiga Research OS
#   $Id$
#
#   Desc: Makefile to create the gensets
#

USER_CFLAGS := -Wall -Wunused -O2
-include $(TOP)/config/make.cfg

HOST_CC	     ?= $(CC)
HOST_STRIP   ?= strip
MECHO	     ?= echo
GENSETS	     ?= gensets
HOST_CFLAGS  ?= $(USER_CFLAGS)
COLLECT-AROS ?= collect-aros

all : $(COLLECT-AROS)

$(COLLECT-AROS) : collect-aros.c gensets.c
	@$(MECHO) "Compiling $(notdir $@)..."
	$(HOST_CC) $(HOST_CFLAGS) $^ -o $@ -DLINKERPATH=\"/usr/bin/ld\" \
	-DNMPATH=\"$(NM_PLAIN)\" -DGCCPATH=\"$(TARGET_CC)\" -DMVPATH=\"$(MV)\"
	@$(HOST_STRIP) $(COLLECT-AROS)

clean :
	@$(RM) $(COLLECT-AROS) 

.PHONY : all clean
