# $Id$
TOP=../..
CURDIR=compiler/include

include $(TOP)/config/make.cfg

AMIGADIR := $(TOP)/amiga

INCLUDES      := $(wildcard *.h */*.h)
DEST_INCLUDES := $(foreach f,$(INCLUDES),$(INCDIR)/$(f))
INCSUBDIRS :=	aros \
		clib \
		defines \
		devices \
		diskfont \
		dos \
		exec \
		gadgets \
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

#MM
includes : make-dirs copy-includes

copy-includes : $(DEST_INCLUDES)

$(INCDIR)/%.h : %.h
	$(CP) $< $@

setup: make-dirs copy-includes

make-dirs :
	@for dir in $(INCDIR)         \
		$(foreach f,$(INCSUBDIRS),$(INCDIR)/$(f)) \
	; do \
	   if [ ! -d "$$dir" ]; then $(MKDIR) "$$dir" ; else true ; fi ;\
	done

#MM
clean :
	-$(RM) $(INCDIR)
