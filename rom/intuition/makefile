# $Id$
TOP=..

SPECIAL_CFLAGS = -I/usr/include

include $(TOP)/config/make.cfg

FILES = intuition_init intuition_functable gadgets \
	boolgadgets propgadgets boopsigadgets \
	imageclass frameiclass icclass buttongclass gadgetclass
FUNCTIONS = \
	activatewindow \
	addclass \
	autorequest \
	beginrefresh \
	clearmenustrip \
	closescreen \
	closewindow \
	disposeobject \
	dogadgetmethoda \
	drawborder \
	drawimage \
	drawimagestate \
	endrefresh \
	eraseimage \
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
	obtaingirport \
	openscreen \
	openscreentaglist \
	openwindow \
	openwindowtaglist \
	pointinimage \
	printitext \
	refreshgadgets \
	refreshglist \
	refreshwindowframe \
	releasegirport \
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

