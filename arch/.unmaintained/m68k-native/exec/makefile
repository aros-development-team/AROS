# $Id$
TOP=../../..

include $(TOP)/config/make.cfg

SPECIAL_AFLAGS = -I../
CLIBDIR=$(GENDIR)/clib

MACHINE_H = $(INCDIR)/aros/machine.h

FILES = disable forbid getcc \
	cacheclearu cachepredma cachepostdma

all: setup \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o) 

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi

clean:
	@true

$(OSGENDIR)/%.o: %.s ../machine.i
	$(AS) $(AFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OSGENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

#ifneq ("$(TARGET)","clean")
#include $(OSGENDIR)/geninc.d $(OSGENDIR)/preparecontext.d
#endif
