#
#   (C) 1995-96 AROS - The Amiga Replacement OS
#   $Id$
#
#   Desc: Amiga bootloader -- makefile
#   Lang: english
#

# This directory is not built like the other parts of AROS.

TOP=../../..

include $(TOP)/config/make.cfg

# Flags to use for the module loader
LDFLAGS = -noixemul

FILES = main ils config

all:	setup $(BINDIR)/boot

clean:
	-rm -f $(BINDIR)/boot

setup :
	@if [ ! -d $(BINDIR) ]; then $(MKDIR) $(BINDIR) ; else true ; fi
	@if [ ! -d $(BOOTGENDIR) ]; then $(MKDIR) $(BOOTGENDIR) ; else true ; fi

$(BOOTGENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(BINDIR)/boot: $(foreach f,$(FILES),$(BOOTGENDIR)/$(f).o)
	$(CC) $(LDFLAGS) $(foreach f,$(FILES),$(BOOTGENDIR)/$(f).o) -o $@
	strip $@

$(BOOTGENDIR)/%.d: %.c
	@if [ ! -d $(BOOTGENDIR) ] ; then $(MKDIR) $(BOOTGENDIR) ; else true ; fi
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -f$@ -p$(BOOTGENDIR) -- $(CFLAGS) -- $^

include $(foreach f,$(FILES),$(BOOTGENDIR)/$(f).d)
