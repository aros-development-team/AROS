# $Id$
TOP=..

include $(TOP)/config/make.cfg

ifeq ($(FLAVOUR),native)
SUBDIRS=
else
SUBDIRS=c demos devs libs
endif

all: $(SDIR)/Startup-Sequence
	@for dir in $(SUBDIRS) ; do \
	    echo "Making all in $(CURDIR)/$$dir..." ; \
	    if ( cd $$dir ; \
		$(MAKE) $(MFLAGS) TOP="$(TOP)/.." CURDIR="$(CURDIR)/$$dir" \
		all ) ; \
	    then echo -n ; else exit 1 ; fi ; \
	done

clean:
	@for dir in $(SUBDIRS) ; do \
	    echo "Making all in $(CURDIR)/$$dir..." ; \
	    $(MAKE) $(MFLAGS) TOP="$(TOP)/.." CURDIR="$(CURDIR)/$$dir" \
		clean ) ; \
	done

$(SDIR)/Startup-Sequence : s/Startup-Sequence
	$(CP) $^ $@


