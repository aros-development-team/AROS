# $Id$
TOP=../..
COMPILE_KERNEL = yes # Must have a value, no matter which one

include $(TOP)/config/make.cfg

SUBDIRS=filesys

FILES = console cdinputhandler rawkeyconvert

all: setup $(foreach f,$(FILES),$(OSGENDIR)/$(f).o)
	@for dir in $(SUBDIRS) ; do \
	    echo "Making all in $(CURDIR)/$$dir..." ; \
	    if ( cd $$dir ; \
		$(MAKE) $(MFLAGS) TOP="$(TOP)/.." CURDIR="$(CURDIR)/$$dir" \
		all ) ; \
	    then echo -n ; else exit 1 ; fi ; \
	done

clean:
	$(RM) $(OSGENDIR)
	@for dir in $(SUBDIRS) ; do \
	    echo "Making all in $(CURDIR)/$$dir..." ; \
	    ( cd $$dir ; \
	    $(MAKE) $(MFLAGS) TOP="$(TOP)/.." CURDIR="$(CURDIR)/$$dir" \
		clean ) ; \
	done

setup:
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi

$(OSGENDIR)/%.o: %.c
	$(CC) $(SHARED_CFLAGS) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OSGENDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

ifneq ($(TARGET),clean)
include $(foreach f,$(FILES),$(OSGENDIR)/$(f).d)
endif
