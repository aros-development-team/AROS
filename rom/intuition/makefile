# $Id$
TOP=..

SPECIAL_CFLAGS = -I/usr/include

include $(TOP)/make.cfg

FILES = intuition_init intuition_functable intui_x11 gadgets \
	boolgadgets propgadgets
FUNCTIONS = \
	activatewindow \
	addclass \
	autorequest \
	beginrefresh \
	clearmenustrip \
	closescreen \
	closewindow \
	disposeobject \
	drawborder \
	drawimage \
	endrefresh \
	freeclass \
	freescreendrawinfo \
	getattr \
	getdefaultpubscreen \
	getscreendata \
	getscreendrawinfo \
	lockibase \
	lockpubscreen \
	makeclass \
	modifyidcmp \
	modifyprop \
	newmodifyprop \
	newobjecta \
	openscreen \
	openscreentaglist \
	openwindow \
	openwindowtaglist \
	printitext \
	refreshgadgets \
	refreshglist \
	refreshwindowframe \
	removeclass \
	setattrsa \
	setdefaultpubscreen \
	setwindowtitles \
	sizewindow \
	unlockibase \
	unlockpubscreen \
	windowtoback \
	windowtofront

all: setup \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o) \
	$(foreach f,$(FUNCTIONS),$(OSGENDIR)/$(f).o)

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi

clean:
	$(RM) $(OSGENDIR) *.err

$(OSGENDIR)/intui_x11.o: intui_x11.c
	$(CC) $(CFLAGS) $(X11CCFLAGS) $< -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OSGENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err ; else true ; fi

intuition_functable.c : $(foreach f,$(FUNCTIONS),$(f).c) \
	    $(TOP)/scripts/makefunctable.awk
	gawk -f $(TOP)/scripts/makefunctable.awk \
	    --assign lib=Intuition \
	    $^

$(OSGENDIR)/%.d: %.c
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -f$@ -p$(OSGENDIR)/ -- $(CFLAGS) -- $^

include $(foreach f,$(FILES),$(OSGENDIR)/$(f).d)
include $(foreach f,$(FUNCTIONS),$(OSGENDIR)/$(f).d)

