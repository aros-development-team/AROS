# $Id$
TOP=../..

include $(TOP)/config/make.cfg

ifeq ($(FLAVOUR),native)
SUBDIRS=
else
SUBDIRS=icon iffparse
endif

OBJDIR = $(GENDIR)/libs
FILES = dummy

all: setup $(foreach f,$(FILES),$(SLIBDIR)/$f.library)
	@for dir in $(SUBDIRS) ; do \
	    echo "Making all in $(CURDIR)/$$dir..." ; \
	    if ( cd $$dir ; \
		$(MAKE) $(MFLAGS) TOP="$(TOP)/.." CURDIR="$(CURDIR)/$$dir" \
		all ) ; \
	    then echo -n ; else exit 1 ; fi ; \
	done

setup :
	@if [ ! -d $(SLIBDIR) ]; then $(MKDIR) $(SLIBDIR) ; else true ; fi
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi

clean:
	-$(RM) $(SLIBDIR)/dummy.library $(OBJDIR)
	@for dir in $(SUBDIRS) ; do \
	    echo "Making all in $(CURDIR)/$$dir..." ; \
	    ( cd $$dir ; \
	    $(MAKE) $(MFLAGS) TOP="$(TOP)/.." CURDIR="$(CURDIR)/$$dir" \
		clean ) ; \
	done

$(SLIBDIR)/%.library: %lib.o
	$(CC) $(ILDFLAGS) $^ -o $@

$(OBJDIR)/%.o: %.c
	$(CC) $(CCFLAGS) -I $(TOP) $< -c -o $@

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

ifneq ($(TARGET),clean)
include $(foreach f,$(FILES),$(OBJDIR)/$(f)lib.d)
endif
