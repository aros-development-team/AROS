# $Id$
TOP=..

include $(TOP)/config/make.cfg

SUBDIRS=
TARGET=all

all: subdirs

clean:
	@$(MAKE) $(MFLAGS) subdirs TARGET=clean

setup:
	@$(MAKE) $(MFLAGS) subdirs TARGET=setup

subdirs:
	@for dir in $(SUBDIRS) ; do \
	    echo "Making $(TARGET) in $(CURDIR)/$$dir..." ; \
	    if ( cd $$dir ; \
		$(MAKE) $(MFLAGS) TOP="$(TOP)/.." CURDIR="$(CURDIR)/$$dir" \
		TARGET=$(TARGET) $(TARGET) ) ; \
	    then true ; else exit 1 ; fi ; \
	done

