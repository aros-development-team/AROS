# $Id$
TOP=../../..

include $(TOP)/config/make.cfg

SPECIAL_AFLAGS = -I../
CLIBDIR=$(GENDIR)/clib

MACHINE_H = $(INCDIR)/aros/machine.h

CFILES = setjmp longjmp

all: setup \
	$(foreach f,$(CFILES),$(CLIBDIR)/$(f).o)

setup :
	@if [ ! -d $(CLIBDIR) ]; then $(MKDIR) $(CLIBDIR) ; else true ; fi

clean:
	@true

$(CLIBDIR)/%.o: %.s ../machine.i
	$(AS) $(AFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

