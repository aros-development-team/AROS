# $Id$
TOP=../..

SPECIAL_CFLAGS = -I/usr/include

include $(TOP)/config/make.cfg

OBJDIR=$(OSGENDIR)/intuition
LIB=$(LIBDIR)/libintuition.so

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
	setgadgetattrsa \
	setpointer \
	setwindowtitles \
	sizewindow \
	unlockibase \
	unlockpubscreen \
	windowtoback \
	windowtofront

OBJS=$(foreach f,$(FILES) $(FUNCTIONS),$(OBJDIR)/$(f).o)

all: setup $(LIB)

$(LIB) : $(OBJS)
	@echo "Recreating $@..."
	@$(SHARED_AR) $@ $(OBJS)

setup :
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi
	@echo "CFLAGS=$(CFLAGS)"

clean:
	$(RM) $(OBJDIR) *.err

$(OBJDIR)/%.o: %.c
	@echo "Compiling $(CURDIR)/$<"
	@$(CC) $(SHARED_CFLAGS)  $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

intuition_functable.c : $(foreach f,$(FUNCTIONS),$(f).c) \
	    $(TOP)/scripts/makefunctable.awk
	gawk -f $(TOP)/scripts/makefunctable.awk \
	    --assign lib=Intuition \
	    $^

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	@echo "Makedepend $(CURDIR)/$<"
	@$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

ifneq ($(TARGET),clean)
include $(foreach f,$(FILES) $(FUNCTIONS),$(OBJDIR)/$(f).d)
endif
