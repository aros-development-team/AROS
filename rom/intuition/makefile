TOP=..

include $(TOP)/make.cfg

FILES = intuition_init intuition_functable intui_x11
FUNCTIONS = \
	addclass \
	activatewindow \
	closescreen \
	closewindow \
	disposeobject \
	drawborder \
	drawimage \
	freeclass \
	getattr \
	makeclass \
	modifyidcmp \
	modifyprop \
	newobjecta \
	openscreen \
	openscreentaglist \
	openwindow \
	openwindowtaglist \
	printitext \
	refreshgadgets \
	refreshglist \
	removeclass \
	setattrsa \
	setwindowtitles \
	sizewindow

all: setup \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o) \
	$(foreach f,$(FUNCTIONS),$(OSGENDIR)/$(f).o)

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; fi

clean:
	$(RM) $(OSGENDIR) *.err

$(OSGENDIR)/intui_x11.o: intui_x11.c
	$(CC) $(CFLAGS) $(X11CCFLAGS) $< -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

$(OSGENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

intuition_functable.c : $(foreach f,$(FUNCTIONS),$(f).c) \
	    $(TOP)/scripts/makefunctable.awk
	gawk -f $(TOP)/scripts/makefunctable.awk \
	    --assign lib=Intuition \
	    $^

$(OSGENDIR)/%.d: %.c
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; fi
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -f$@ -p$(OSGENDIR)/ -- $(CFLAGS) -- $^

include $(foreach f,$(FILES),$(OSGENDIR)/$(f).d)
include $(foreach f,$(FUNCTIONS),$(OSGENDIR)/$(f).d)

