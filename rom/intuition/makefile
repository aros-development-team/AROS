TOP=..

include $(TOP)/make.cfg

FILES = addclass activatewindow closewindow disposeobject drawborder \
	drawimage freeclass getattr makeclass modifyidcmp modifyprop \
	newobjecta openwindow printitext refreshgadgets refreshglist \
	removeclass setattrsa setwindowtitles sizewindow

all: setup $(OSGENDIR)/intuition_init.o $(OSGENDIR)/intuition_functable.o \
	$(OSGENDIR)/intui_x11.o \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o)

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

intuition_functable.c : $(foreach f,$(FILES),$(f).c) \
	    $(TOP)/scripts/makefunctable.awk
	gawk -f $(TOP)/scripts/makefunctable.awk \
	    --assign lib=Intuition \
	    $(foreach f,$(FILES),$(f).c)

$(OSGENDIR)/%.d: %.c
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -f$@ -p$(OSGENDIR)/ -- $(CFLAGS) -- $^

include $(foreach f,$(FILES),$(OSGENDIR)/$(f).d)

