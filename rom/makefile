# $Id$
TOP=..

include $(TOP)/config/make.cfg

ifeq ($(FLAVOUR),native)
# Only exec, for the moment.
SUBDIRS=exec
else
SUBDIRS=exec utility dos graphics intuition devs
endif

all:
	@for dir in $(SUBDIRS) ; do \
	    echo "Making all in $(CURDIR)/$$dir..." ; \
	    if ( cd $$dir ; \
		$(MAKE) $(MFLAGS) TOP="$(TOP)/.." CURDIR="$(CURDIR)/$$dir" \
		all ) ; \
	    then echo -n ; else exit 1 ; fi ; \
	done

clean:
	@for dir in $(SUBDIRS) ; do \
	    echo "Cleaning in $(CURDIR)/$$dir..." ; \
	    ( cd $$dir ; \
	    $(MAKE) $(MFLAGS) TOP="$(TOP)/.." CURDIR="$(CURDIR)/$$dir" \
		TARGET=clean clean ) ; \
	done

setup:
	@true
