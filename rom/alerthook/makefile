# $Id$
TOP=../..
COMPILE_KERNEL = yes # Must have a value, no matter which one

# Use OS's own includes over the ones from AROS
SPECIAL_CFLAGS=-I/usr/include

include $(TOP)/config/make.cfg

FILES = alerthook

ifeq ($(FLAVOUR),native)
all: setup \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o) \
	$(foreach f,$(FUNCTIONS),$(OSGENDIR)/$(f).o) \
	$(foreach f,$(FUNCTIONS),$(f).s) \
	$(OSMODDIR)/alert.hook

$(OSMODDIR)/alert.hook: $(foreach f,$(FILES),$(OSGENDIR)/$(f).o) \
		       $(foreach f,$(FUNCTIONS),$(OSGENDIR)/$(f).o)
	$(CC) $(ILDFLAGS) \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o) \
	-o $@ 2>&1|tee alerthook.err
	@if test ! -s alerthook.err; then rm alerthook.err ; else true ; fi
	@strip $@
else
all: setup \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o) \
	$(foreach f,$(FUNCTIONS),$(OSGENDIR)/$(f).o)

endif

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi
	@if [ ! -d $(OSMODDIR) ]; then $(MKDIR) $(OSMODDIR) ; else true ; fi

clean:

$(OSGENDIR)/%.o: %.c
	$(CC) $(SHARED_CFLAGS) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OSGENDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

ifneq ($(TARGET),clean)
include $(foreach f,$(FILES) $(FUNCTIONS),$(OSGENDIR)/$(f).d)
endif
