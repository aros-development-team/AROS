TOP=..

include $(TOP)/make.cfg

AMIGADIR=$(TOP)/amiga
INCDIR=$(ARCHDIR)/include

all: setup links

links:
	$(TOP)/scripts/makelinks . $(INCDIR) "*.h"
	touch links

setup:
	@if [ ! -d $(INCDIR) ]; then $(MKDIR) $(INCDIR) ; else true ; fi

clean:
	$(RM) $(INCDIR) links
