# $Id$
TOP=..

include $(TOP)/config/make.cfg

AMIGADIR=$(TOP)/amiga
OBJDIR=$(GENDIR)/include
INCDIR=$(BINDIR)/include

all: setup $(OBJDIR)/links

$(OBJDIR)/links:
	$(TOP)/scripts/makelinks . $(INCDIR) "*.h"
	touch $(OBJDIR)/links

setup:
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi
	@if [ ! -d $(INCDIR) ]; then $(MKDIR) $(INCDIR) ; else true ; fi

clean:
	$(RM) $(INCDIR) links
