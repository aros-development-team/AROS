# $Id$
TOP=../..

include $(TOP)/config/make.cfg

SUBDIRS=exec utility clib

SPECIAL_CFLAGS = -I $(TOP)/exec

MACHINE_H = $(INCDIR)/aros/machine.h

FILES = 

all: setup $(MACHINE_H) machine.i subdirs \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o)

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi

subdirs :
	@for dir in $(SUBDIRS) ; do \
	    echo "Making $(TARGET) in $(CURDIR)/$$dir..." ; \
	    if ( cd $$dir ; \
		$(MAKE) $(MFLAGS) TOP="$(TOP)/.." CURDIR="$(CURDIR)/$$dir" \
		TARGET=$(TARGET) $(TARGET) ) ; \
	    then true ; else exit 1 ; fi ; \
	done

$(MACHINE_H): machine.h
	$(CP) $< $@

clean:
	$(RM) $(MACHINE_H) machine.i geninc
	@for dir in $(SUBDIRS) ; do \
	    echo "Cleaning in $(CURDIR)/$$dir..." ; \
	    cd $$dir ; \
	    $(MAKE) $(MFLAGS) TOP="$(TOP)/.." CURDIR="$(CURDIR)/$$dir" \
		TARGET=clean clean ; \
	done
	$(RM) host.cfg $(CONFIG_H) $(INCDIR)/aros

machine.i: geninc.c
	$(CC) $(CFLAGS) -o geninc geninc.c
	./geninc > machine.i

$(OSGENDIR)/%.o: %.s machine.i
	$(AS) $(AFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OSGENDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

