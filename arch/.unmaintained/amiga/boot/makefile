#
#   (C) 1995-96 AROS - The Amiga Replacement OS
#   $Id$
#
#   Desc: Amiga bootloader -- makefile
#   Lang: english
#

# The files in this directory build a stand-alone executable, used to
# install AROS' modules in the system. These modules will become active
# after a reset. More information in the source files.

TOP=../../..

include $(TOP)/config/make.cfg

TOOLDIR = $(BINDIR)/tools
TOOLGENDIR = $(GENDIR)/tools

# Flags to use for the module loader
LDFLAGS = -noixemul

FILES = main ils config
TOOLZ = printresmodules showvecs
TOOLS = $(foreach f,$(TOOLZ),$(TOOLDIR)/$(f))
LIBS = -L$(LIBDIR) -larosc

all:	setup $(BINDIR)/boot $(BINDIR)/boot.config $(TOOLS)

clean:
	-rm -f $(BINDIR)/boot

setup :
	@if [ ! -d $(BINDIR) ]; then $(MKDIR) $(BINDIR) ; else true ; fi
	@if [ ! -d $(BOOTGENDIR) ]; then $(MKDIR) $(BOOTGENDIR) ; else true ; fi
	@if [ ! -d $(TOOLDIR) ]; then $(MKDIR) $(TOOLDIR) ; else true ; fi
	@if [ ! -d $(TOOLGENDIR) ]; then $(MKDIR) $(TOOLGENDIR) ; else true ; fi

$(BOOTGENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(TOOLGENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(BINDIR)/boot: $(foreach f,$(FILES),$(BOOTGENDIR)/$(f).o)
	$(CC) $(LDFLAGS) $(foreach f,$(FILES),$(BOOTGENDIR)/$(f).o) $(LIBS) -o $@
	strip $@

$(BINDIR)/boot.config: boot.config
	$(CP) $< $@

$(TOOLDIR)/%: $(TOOLGENDIR)/%.o
	$(CC) $(LDFLAGS) $< -o $@
	strip $@

$(BOOTGENDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

include $(foreach f,$(FILES),$(BOOTGENDIR)/$(f).d)
