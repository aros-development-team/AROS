# $Id$
TOP=../..

include $(TOP)/config/make.cfg

AMIGADIR=$(TOP)/amiga

INCLUDES := $(wildcard *.h */*.h)
DEST_INCLUDES := $(foreach f,$(INCLUDES),$(INCDIR)/$(f))
INCSUBDIRS :=	aros \
		clib \
		defines \
		devices \
		diskfont \
		dos \
		exec \
		graphics \
		hardware \
		inline \
		intuition \
		libraries \
		pragmas \
		prefs \
		proto \
		utility \
		workbench

all: setup

os-include: $(DEST_INCLUDES)

$(INCDIR)/%.h: %.h
	$(CP) $< $@

setup: make-dirs os-include

make-dirs:
	@for dir in $(INCDIR)         \
		$(foreach f,$(INCSUBDIRS),$(INCDIR)/$(f)) \
	; do \
	   if [ ! -d "$$dir" ]; then $(MKDIR) "$$dir" ; else true ; fi ;\
	done

clean:
	-$(RM) $(INCDIR)
