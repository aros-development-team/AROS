#
#   (C) 1995-96 AROS - The Amiga Replacement OS
#   $Id$
#
#   Desc: Amiga bootloader -- makefile
#   Lang: english
#

# The files in this directory build a stand-alone executable, used to
# install AROS' modules in the system. These modules will become active
# after a reset. More information in the source and doc files.

TOP=../../..

include $(TOP)/config/make.cfg

# Set this to "true" to prevent stripping of the executables.
STRIP = strip

# Directories where the resulting files will end up
BOOTGENDIR = $(GENDIR)/boot
TOOLDIR = $(BINDIR)/tools
TOOLGENDIR = $(GENDIR)/tools

# Flags to use for the module loader
LDFLAGS = -noixemul

FILES = main ils config
TOOLZ = printresmodules showvecs
TOOLS = $(foreach f,$(TOOLZ),$(TOOLDIR)/$(f))
LIBS = -L$(LIBDIR) -larosc

.SUFFIXES:              # Delete the default suffixes
.SUFFIXES: .c .o .h .d  # Define our suffix list

.PHONY: all clean setup

all:	setup \
	$(BINDIR)/arosboot $(BINDIR)/arosboot.config $(BINDIR)/arosboot.doc \
	$(TOOLS)

clean:
	-rm -f $(BINDIR)/arosboot $(BINDIR)/arosboot.config $(BINDIR)/arosboot.doc
	-rm -f $(TOOLS)

setup :
	@if [ ! -d $(BINDIR) ]; then $(MKDIR) $(BINDIR) ; else true ; fi
	@if [ ! -d $(BOOTGENDIR) ]; then $(MKDIR) $(BOOTGENDIR) ; else true ; fi
	@if [ ! -d $(TOOLDIR) ]; then $(MKDIR) $(TOOLDIR) ; else true ; fi
	@if [ ! -d $(TOOLGENDIR) ]; then $(MKDIR) $(TOOLGENDIR) ; else true ; fi

$(BOOTGENDIR)/%.o: %.c
	@$(ECHO) "Compiling $<..."
	@$(CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if [ ! -s $*.err ]; then rm $*.err ; else true ; fi

$(TOOLGENDIR)/%.o: %.c
	@$(ECHO) "Compiling $<..."
	@$(CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if [ ! -s $*.err ]; then rm $*.err ; else true ; fi

$(BINDIR)/arosboot: $(foreach f,$(FILES),$(BOOTGENDIR)/$(f).o)
	@$(ECHO) "Building $@..."
	@$(CC) $(LDFLAGS) $(foreach f,$(FILES),$(BOOTGENDIR)/$(f).o) $(LIBS) -o $@
	@$(STRIP) $@

$(BINDIR)/arosboot.config: arosboot.config
	@$(ECHO) "Updating $@..."
	@$(CP) $< $@

$(BINDIR)/arosboot.doc: arosboot.doc
	@$(ECHO) "Updating $@..."
	@$(CP) $< $@

$(TOOLDIR)/%: $(TOOLGENDIR)/%.o
	@$(ECHO) "Building $@..."
	@$(CC) $(LDFLAGS) $< -o $@
	@$(STRIP) $@

$(BOOTGENDIR)/%.d: %.c
	@$(ECHO) "Makedepend $<..."
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	@$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

$(TOOLGENDIR)/%.d: %.c
	@$(ECHO) "Makedepend $<..."
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	@$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

# Don't try to include dependency files if we're cleaning up.
ifneq ("$(TARGET)","clean")
include $(foreach f,$(FILES),$(BOOTGENDIR)/$(f).d)
include $(foreach f,$(TOOLZ),$(TOOLGENDIR)/$(f).d)
endif
