# $Id$
TOP=../..

SPECIAL_CFLAGS = -I/usr/include

include $(TOP)/config/make.cfg

FILES = intuition_init intuition_functable gadgets \
	boolgadgets propgadgets boopsigadgets \
	imageclass frameiclass icclass buttongclass gadgetclass
FUNCTIONS = \
	activatewindow \
	addclass \
	addgadget \
	addglist \
	allocremember \
	autorequest \
	beginrefresh \
	changewindowbox \
	clearmenustrip \
	closescreen \
	closewindow \
	disposeobject \
	dogadgetmethoda \
	doubleclick \
	drawborder \
	drawimage \
	drawimagestate \
	easyrequestargs \
	endrefresh \
	eraseimage \
	freeclass \
	freeremember \
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
	movescreen \
	movewindow \
	newmodifyprop \
	newobjecta \
	obtaingirport \
	offgadget \
	ongadget \
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
	removegadget \
	removeglist \
	reportmouse \
	screentofront \
	setattrsa \
	setdefaultpubscreen \
	setpointer \
	setwindowtitles \
	sizewindow \
	unlockibase \
	unlockpubscreen \
	windowtoback \
	windowtofront

all: setup \
	$(foreach f,$(FILES) $(FUNCTIONS),$(OSGENDIR)/$(f).o)

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi

clean:
	$(RM) $(OSGENDIR) *.err

$(OSGENDIR)/%.o: %.c
	$(CC) $(SHARED_CFLAGS)  $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

intuition_functable.c : $(foreach f,$(FUNCTIONS),$(f).c) \
	    $(TOP)/scripts/makefunctable.awk
	gawk -f $(TOP)/scripts/makefunctable.awk \
	    --assign lib=Intuition \
	    $^

$(OSGENDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

ifneq ($(TARGET),clean)
include $(foreach f,$(FILES) $(FUNCTIONS),$(OSGENDIR)/$(f).d)
endif
