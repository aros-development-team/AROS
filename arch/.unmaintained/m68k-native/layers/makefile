# $Id$
TOP=../../..

include $(TOP)/config/make.cfg

SPECIAL_AFLAGS = -I../
CLIBDIR=$(GENDIR)/clib
OBJDIR=$(OSGENDIR)/layers

MACHINE_H = $(INCDIR)/aros/machine.h

FILES = basicasmfuncs

all: setup \
	$(foreach f,$(FILES),$(OBJDIR)/$(f).o) 

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi
	@echo "AFLAGS=$(AFLAGS)"

clean:
	@true

$(OBJDIR)/%.o: %.s ../machine.i
	@echo "Assembling $<..."
	@$(AS) $(AFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

