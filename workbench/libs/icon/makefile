# $Id$
TOP=../../..

USER_INCLUDES = -I..

include $(TOP)/config/make.cfg

OBJDIR=$(GENDIR)/workbench/libs/icon

FILES = icon_init icon_functable basicfuncs
FUNCTIONS = \
	addfreelist \
	bumprevision \
	deletediskobject \
	findtooltype \
	freefreelist \
	getdefdiskobject \
	getdiskobjectnew \
	matchtoolvalue \
	putdefdiskobject

all: setup $(SLIBDIR)/icon.library

$(SLIBDIR)/icon.library: $(foreach f,$(FILES) $(FUNCTIONS),$(OBJDIR)/$(f).o)
	$(CC) $(ILDFLAGS) $^ -o $@

setup :
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi
	@if [ ! -d $(SLIBDIR) ]; then $(MKDIR) $(SLIBDIR) ; else true ; fi

clean:
	$(RM) $(OBJDIR) *.err

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

icon_functable.c : $(foreach f,$(FUNCTIONS),$(f).c) \
	    $(TOP)/scripts/makefunctable.awk
	gawk -f $(TOP)/scripts/makefunctable.awk \
	    --assign lib=Icon \
	    $^

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

ifneq ($(TARGET),clean)
include $(foreach f,$(FILES) $(FUNCTIONS),$(OBJDIR)/$(f).d)
endif
