# $Id$
TOP=../..

include $(TOP)/config/make.cfg

SUBDIRS=countries
LOCALEDIR=$(BINDIR)/Locale

all: setup
	@for dir in $(SUBDIRS) ; do \
	    echo "Making all in $(CURDIR)/$$dir..." ; \
	    if ( cd $$dir ; \
		$(MAKE) $(MFLAGS) TOP="$(TOP)/.." CURDIR="$(CURDIR)/$$dir" \
		all ) ; \
	    then echo -n ; else exit 1 ; fi ; \
	done

setup :
	@if [ ! -d $(LOCALEDIR) ]; then $(MKDIR) $(LOCALEDIR); else true; fi

clean:
	@for dir in $(SUBDIRS) ; do \
	    echo "Making all in $(CURDIR)/$$dir..." ; \
	    ( cd $$dir ; \
	    $(MAKE) $(MFLAGS) TOP="$(TOP)/.." CURDIR="$(CURDIR)/$$dir" \
		clean ) ; \
	done
