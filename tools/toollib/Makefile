#   Copyright © 1995-2001, The AROS Development Team. All rights reserved.
#   $Id$
#
#   Desc: Make the tool library.
#
SRC	= error filesup hash lineparser mystream \
	  stdiocb stringcb toollib vstring

# Include the AROS build information. This will fail if $(TOP) is not
# set, and build in the current directory.
-include $(TOP)/config/make.cfg

# If the include above doesn't work, we need some defaults.
HOST_CC	?= $(CC)
HOST_AR ?= $(AR) cru 
TOOLDIR ?= .
ifneq ($(HOSTGENDIR),)
OBJDIR := $(HOSTGENDIR)/$(CURDIR)
else
OBJDIR := $(TOOLDIR)/obj
endif
MKDIR ?= mkdir
MECHO ?= echo
TOOLLIB ?= $(TOOLDIR)/libtool.a
HOST_CFLAGS := $(HOST_CFLAGS) -Wall -g -DHAVE_VSNPRINTF -I.. -O


DEPS   := $(foreach f,$(SRC),$(OBJDIR)/$(f).d)

all : depend $(OBJDIR) $(TOOLLIB)

$(TOOLLIB) : $(foreach f,$(SRC),$(OBJDIR)/$(f).o)
	@$(MECHO) "Creating toollib/$(notdir $@)..."
	@$(HOST_AR) $@ $?

$(OBJDIR)/%.o : %.c
	@$(MECHO) "Compiling toollib/$(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) -o $@ -c $<

$(OBJDIR)/%.d : %.c
	@$(MECHO) Finding dependencies for $<...
	@$(MKDIR) -p $(dir $@)
	@$(HOST_CC) $(HOST_CFLAGS) -M $< > $@

$(OBJDIR) : 
	@$(MKDIR) $(OBJDIR)

clean :
	-@$(RM) -r $(OBJDIR) $(TOOLLIB)
	@sed '/^# DO NOT DELETE$$/,$$d' Makefile > Makefile.tmp
	@echo "# DO NOT DELETE" >> Makefile.tmp
	@mv -f Makefile.tmp Makefile
	-@$(RM) Makefile.bak

depend : $(DEPS)

-include $(DEPS)
# DO NOT DELETE
