# $Id$
TOP=../..

include $(TOP)/config/make.cfg

OBJDIR=$(GENDIR)/libs

FILES = dummy

all: setup $(foreach f,$(FILES),$(SLIBDIR)/$f.library)

setup :
	@if [ ! -d $(SLIBDIR) ]; then $(MKDIR) $(SLIBDIR) ; else true ; fi
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi

clean:
	-$(RM) $(SLIBDIR)/dummy.library $(OBJDIR)

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
