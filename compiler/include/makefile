# $Id$
TOP=../..

include $(TOP)/config/make.cfg

AMIGADIR=$(TOP)/amiga

INCLUDES = $(wildcard *.h */*.h)
DEST_INCLUDES = $(foreach f,$(INCLUDES),$(INCDIR)/$(f))

all: setup

os-include: $(DEST_INCLUDES)

$(INCDIR)/%.h: %.h
	$(CP) $< $@

setup: make-dirs os-include

make-dirs:
<<<<<<< makefile
	@for dir in $(INCDIR)         \
		$(INCDIR)/aros        \
		$(INCDIR)/clib        \
		$(INCDIR)/defines     \
		$(INCDIR)/devices     \
		$(INCDIR)/dos         \
		$(INCDIR)/exec        \
		$(INCDIR)/graphics    \
		$(INCDIR)/hardware    \
		$(INCDIR)/inline      \
		$(INCDIR)/intuition   \
		$(INCDIR)/libraries   \
		$(INCDIR)/pragmas     \
		$(INCDIR)/prefs       \
		$(INCDIR)/proto       \
		$(INCDIR)/utility     \
		$(INCDIR)/workbench   \
	; do \
	   if [ ! -d "$$dir" ]; then $(MKDIR) "$$dir" ; else true ; fi ;\
	done

clean:
	-$(RM) $(INCDIR)
