# $Id$
TOP=../..

include $(TOP)/config/make.cfg

AMIGADIR=$(TOP)/amiga
INCDIR=$(BINDIR)/include

INCLUDES = $(wildcard *.h */*.h)
DEST_INCLUDES = $(foreach f,$(INCLUDES),$(INCDIR)/$(f))

all: setup

os-include: $(DEST_INCLUDES)

$(INCDIR)/%.h: %.h
	$(CP) $< $@

setup: make-dirs os-include

make-dirs:
	@if [ ! -d $(INCDIR) ]; then $(MKDIR) $(INCDIR) ; else true ; fi
	@if [ ! -d $(INCDIR)/aros ]; then $(MKDIR) $(INCDIR)/aros ; else true ; fi
	@if [ ! -d $(INCDIR)/clib ]; then $(MKDIR) $(INCDIR)/clib ; else true ; fi
	@if [ ! -d $(INCDIR)/defines ]; then $(MKDIR) $(INCDIR)/defines ; else true ; fi
	@if [ ! -d $(INCDIR)/dos ]; then $(MKDIR) $(INCDIR)/dos ; else true ; fi
	@if [ ! -d $(INCDIR)/exec ]; then $(MKDIR) $(INCDIR)/exec ; else true ; fi
	@if [ ! -d $(INCDIR)/graphics ]; then $(MKDIR) $(INCDIR)/graphics ; else true ; fi
	@if [ ! -d $(INCDIR)/intuition ]; then $(MKDIR) $(INCDIR)/intuition ; else true ; fi
	@if [ ! -d $(INCDIR)/pragmas ]; then $(MKDIR) $(INCDIR)/pragmas ; else true ; fi
	@if [ ! -d $(INCDIR)/proto ]; then $(MKDIR) $(INCDIR)/proto ; else true ; fi
	@if [ ! -d $(INCDIR)/utility ]; then $(MKDIR) $(INCDIR)/utility ; else true ; fi

clean:
	-$(RM) $(INCDIR)
