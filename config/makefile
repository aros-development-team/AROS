# $Id$

TOP=..

include $(TOP)/config/make.cfg

SUBDIRS=$(KERNEL) $(ARCH)

all: setup $(TOP)/compiler/include/aros/config.h
	@for dir in $(SUBDIRS) ; do \
	    echo "Making all in $(CURDIR)/$$dir..." ; \
	    if ( cd $$dir ; \
		$(MAKE) $(MFLAGS) TOP="$(TOP)/.." CURDIR="$(CURDIR)/$$dir" \
		all ) ; \
	    then echo -n ; else exit 1 ; fi ; \
	done

config: setup $(TOP)/compiler/include/aros/config.h
	@for dir in $(SUBDIRS) ; do \
	    echo "Configuring in $(CURDIR)/$$dir..." ; \
	    if ( cd $$dir ; \
		$(MAKE) $(MFLAGS) TOP="$(TOP)/.." CURDIR="$(CURDIR)/$$dir" \
		config ) ; \
	    then echo -n ; else exit 1 ; fi ; \
	done

setup:
	@for dir in $(SUBDIRS) ; do \
	    echo "Setting up in $(CURDIR)/$$dir..." ; \
	    if ( cd $$dir ; \
		$(MAKE) $(MFLAGS) TOP="$(TOP)/.." CURDIR="$(CURDIR)/$$dir" \
		setup ) ; \
	    then echo -n ; else exit 1 ; fi ; \
	done

$(TOP)/compiler/include/aros/config.h: $(TOP)/configure
	sh $(TOP)/configure -h $(TOP)/compiler/include/aros/config.h

clean:
	-$(RM) ./machine
	@for dir in $(SUBDIRS) ; do \
	    echo "Cleaning in $(CURDIR)/$$dir..." ; \
	    cd $$dir ; \
	    $(MAKE) $(MFLAGS) TOP="$(TOP)/.." CURDIR="$(CURDIR)/$$dir" \
		TARGET=clean clean ; \
	done
	$(RM) host.cfg config.h

machine:
	$(CC) -I$(TOP)/compiler/include -o machine machine.c
	./machine

