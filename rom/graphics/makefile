TOP=..

include $(TOP)/make.cfg

FILES = closefont draw getapen getbpen getdrmd initrastport move \
	openfont rectfill scrollraster setapen setbpen setdrmd \
	setfont setrast text textlength writepixel

all: setup $(OSGENDIR)/graphics_init.o $(OSGENDIR)/graphics_functable.o \
	$(OSGENDIR)/driver_x11.o \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o)

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; fi

clean:
	$(RM) $(OSGENDIR) *.err

$(OSGENDIR)/driver_x11.o: driver_x11.c
	$(CC) $(CFLAGS) $(X11CCFLAGS) $< -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

$(OSGENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

graphics_functable.c : $(foreach f,$(FILES),$(f).c) \
	    $(TOP)/scripts/makefunctable.awk
	gawk -f $(TOP)/scripts/makefunctable.awk \
	    --assign lib=Graphics \
	    $(foreach f,$(FILES),$(f).c)

$(OSGENDIR)/%.d: %.c
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -f$@ -p$(OSGENDIR)/ -- $(CFLAGS) -- $^

include $(foreach f,$(FILES),$(OSGENDIR)/$(f).d)

