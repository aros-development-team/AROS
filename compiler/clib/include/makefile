# $Id$
TOP=../../../..

include $(TOP)/config/make.cfg

INCDIR=$(BINDIR)/include

INCLUDES = $(wildcard *.h */*.h)
DEST_INCLUDES = $(foreach f,$(INCLUDES),$(INCDIR)/$(f))

all: setup $(DEST_INCLUDES)

setup:
	@if [ ! -d $(INCDIR) ]; then $(MKDIR) $(INCDIR) ; else true ; fi
	@if [ ! -d $(INCDIR)/sys ]; then $(MKDIR) $(INCDIR)/sys ; else true ; fi

$(INCDIR)/%.h : %.h
	$(CP) $< $@

