TOP=..

include $(TOP)/make.cfg

OBJDIR=$(GENDIR)/libs

FILES = dummy

all: setup $(foreach f,$(FILES),$(SLIBDIR)/$f.library)

setup :
	@if [ ! -d $(SLIBDIR) ]; then $(MKDIR) $(SLIBDIR) ; fi
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; fi

clean:
	$(RM) $(SLIBDIR)/dummy.library $(OBJDIR)

$(SLIBDIR)/%.library: %lib.o
	$(CC) $(ILDFLAGS) $^ -o $@

$(OBJDIR)/%.o: %.c
	$(CC) $(CCFLAGS) -I $(TOP) $< -c -o $@

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; fi
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -f$@ -p$(OBJDIR)/ -- $(CFLAGS) -- $^

include $(foreach f,$(FILES),$(OBJDIR)/$(f)lib.d)

