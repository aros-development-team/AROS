# $Id$
TOP=..

include $(TOP)/make.cfg

FILES = utility_init utility_functable
FUNCTIONS = \
	applytagchanges callhookpkt findtagitem nexttagitem \
	stricmp strnicmp tolower toupper getuniqueid smult32 umult32 \
	smult64 umult64 date2amiga amiga2date allocatetagitems \
	freetagitems clonetagitems gettagdata packbooltags \
	refreshtagitemclones taginarray

all: setup \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o) \
	$(foreach f,$(FUNCTIONS),$(OSGENDIR)/$(f).o)

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi

clean:
	$(RM) $(OSGENDIR) *.err $(LIBDIR)/libutility.a

$(OSGENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err ; else true ; fi

utility_functable.c: $(foreach f,$(FUNCTIONS),$(f).c) \
	    $(TOP)/scripts/makefunctable.awk
	gawk -f $(TOP)/scripts/makefunctable.awk \
	    --assign lib=Utility \
	    $^

$(OSGENDIR)/%.d: %.c
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -p$(OSGENDIR)/

include $(foreach f,$(FILES),$(OSGENDIR)/$(f).d)
include $(foreach f,$(FUNCTIONS),$(OSGENDIR)/$(f).d)

