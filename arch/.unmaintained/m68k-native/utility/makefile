# $Id$
TOP=../../..

include $(TOP)/config/make.cfg

SPECIAL_AFLAGS = -I../
CLIBDIR=$(GENDIR)/clib

MACHINE_H = $(INCDIR)/aros/machine.h

FILES = sdivmod32 smult32 smult64 udivmod32 umult32 umult64

all: setup \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o) 

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi

clean:
	@true

$(OSGENDIR)/%.o: %.s ../machine.i
	$(AS) $(AFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

