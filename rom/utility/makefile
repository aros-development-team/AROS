TOP=..

include $(TOP)/make.cfg

FILES = applytagchanges callhookpkt findtagitem nexttagitem \
	stricmp strnicmp tolower toupper getuniqueid smult32 umult32 \
	smult64 umult64 date2amiga amiga2date allocatetagitems \
	freetagitems

all: setup $(OSGENDIR)/utility_init.o $(OSGENDIR)/utility_functable.o \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o)

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; fi

clean:
	$(RM) $(OSGENDIR) *.err $(LIBDIR)/libutility.a

$(OSGENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

utility_functable.c: $(foreach f,$(FILES),$(f).c) \
	    $(TOP)/scripts/makefunctable.awk
	gawk -f $(TOP)/scripts/makefunctable.awk \
	    --assign lib=Utility \
	    $(foreach f,$(FILES),$(f).c)

$(OSGENDIR)/%.d: %.c
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; fi
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -f$@ -p$(OSGENDIR)/ -- $(CFLAGS) -- $^

include $(foreach f,$(FILES),$(OSGENDIR)/$(f).d)

