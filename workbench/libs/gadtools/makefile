# $Id$
TOP=../../..

USER_INCLUDES = -I..

include $(TOP)/config/make.cfg

OBJDIR=$(GENDIR)/workbench/libs/gadtools
DEPLIBS=$(LIBDIR)/libamiga.a $(LIBDIR)/libarosc.a
LIBS=-L$(LIBDIR) -lamiga -larosc

FILES = gadtools_init gadtools_functable basicfuncs

FUNCTIONS = \
        createcontext \
	creategadgeta \
	createmenusa \
	drawbevelboxa \
	freegadgets \
	freevisualinfo \
	getvisualinfoa \
	gt_beginrefresh \
	gt_endrefresh \
	gt_filterimsg \
	gt_getgadgetattrsa \
	gt_getimsg \
	gt_postfilterimsg \
	gt_refreshwindow \
	gt_replyimsg \
	gt_setgadgetattrsa \
	layoutmenuitemsa \
	layoutmenusa

all: setup $(SLIBDIR)/gadtools.library

$(SLIBDIR)/gadtools.library: $(DEPLIBS) \
	    $(foreach f,$(FILES) $(FUNCTIONS),$(OBJDIR)/$(f).o)
	$(CC) $(ILDFLAGS) $^ $(LIBS) -o $@

setup :
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi
	@if [ ! -d $(SLIBDIR) ]; then $(MKDIR) $(SLIBDIR) ; else true ; fi

clean:
	$(RM) $(OBJDIR) *.err

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

gadtools_functable.c : $(foreach f,$(FUNCTIONS),$(f).c) \
	    $(TOP)/scripts/makefunctable.awk
	gawk -f $(TOP)/scripts/makefunctable.awk \
	    --assign lib=Gadtools \
	    $^

$(OBJDIR)/gadtools_init.o: libdefs.h

libdefs.h : lib.conf $(TOP)/scripts/genlibdefs.gawk
	gawk -f $(TOP)/scripts/genlibdefs.gawk lib.conf > libdefs.h

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

ifneq ($(TARGET),clean)
include $(foreach f,$(FILES) $(FUNCTIONS),$(OBJDIR)/$(f).d)
endif
