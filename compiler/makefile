# $Id$
TOP=..

include $(TOP)/config/make.cfg

ifeq ($(FLAVOUR),native)
SUBDIRS=include aros alib clib
else
SUBDIRS=include aros alib clib vcpp vbcc
endif
TARGET=all

all: subdirs

clean:
	@for dir in $(SUBDIRS) ; do \
	    echo "Cleaning in $(CURDIR)/$$dir..." ; \
	    ( cd $$dir ; \
	    $(MAKE) $(MFLAGS) TOP="$(TOP)/.." CURDIR="$(CURDIR)/$$dir" \
		clean ) ; \
	done

setup:
	@$(MAKE) $(MFLAGS) subdirs TARGET=setup

subdirs:
	@for dir in $(SUBDIRS) ; do \
	    echo "Making $(TARGET) in $(CURDIR)/$$dir..." ; \
	    if ( cd $$dir ; \
		$(MAKE) $(MFLAGS) TOP="$(TOP)/.." CURDIR="$(CURDIR)/$$dir" \
		$(TARGET) ) ; \
	    then true ; else exit 1 ; fi ; \
	done

