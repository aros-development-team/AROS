# $Id$

TOP=..

include $(TOP)/config/make.cfg

SUBDIRS=$(KERNEL) $(ARCH)

CONFIG_H = $(INCDIR)/aros/config.h

all: setup
	make subdirs TARGET=all

setup : make-dirs $(CONFIG_H)
	make subdirs TARGET=setup

make-dirs:
	@if [ ! -d $(INCDIR)/aros ]; then $(MKDIR) $(INCDIR)/aros; else true; fi

subdirs :
	@for dir in $(SUBDIRS) ; do \
	    echo "Making $(TARGET) in $(CURDIR)/$$dir..." ; \
	    if ( cd $$dir ; \
		$(MAKE) $(MFLAGS) TOP="$(TOP)/.." CURDIR="$(CURDIR)/$$dir" \
		TARGET=$(TARGET) $(TARGET) ) ; \
	    then true ; else exit 1 ; fi ; \
	done

$(CONFIG_H): make-dirs $(TOP)/configure
	sh $(TOP)/configure -h $@

clean:
	-$(RM) ./machine
	@for dir in $(SUBDIRS) ; do \
	    echo "Cleaning in $(CURDIR)/$$dir..." ; \
	    cd $$dir ; \
	    $(MAKE) $(MFLAGS) TOP="$(TOP)/.." CURDIR="$(CURDIR)/$$dir" \
		TARGET=clean clean ; \
	done
	$(RM) host.cfg $(CONFIG_H) $(INCDIR)/aros

machine:
	$(CC) -I$(TOP)/compiler/include -I$(TOP)/compiler/aros/include -o machine machine.c
	./machine

ifeq ("$(TARGET)","clean")
include $(TOP)/config/host.cfg
endif
