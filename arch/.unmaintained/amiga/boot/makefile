# This directory is not built like the other parts of AROS.

TOP=../../..

include $(TOP)/config/make.cfg

# Flags to use for the module loader
CFLAGS = $(COMMON_CFLAGS) -Wno-uninitialized
LDFLAGS = -noixemul

FILES = main ils

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
	$(MKDEPEND) -f$@ -p$(BOOTGENDIR) -- $(CFLAGS) -I/usr/os-include -- $^

include $(foreach f,$(FILES),$(BOOTGENDIR)/$(f).d)
