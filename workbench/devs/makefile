TOP=..

include $(TOP)/make.cfg

OBJDIR = $(GENDIR)/devs

DEVS = $(DEVSDIR)/ram.handler \
	$(DEVSDIR)/nil.handler

FILES = console cdinputhandler rawkeyconvert

all: setup \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o) \
	$(DEVS)

setup:
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; fi
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; fi
	@if [ ! -d $(DEVSDIR) ]; then $(MKDIR) $(DEVSDIR) ; fi

clean:
	$(RM) $(OSGENDIR)
	$(RM) $(DEVSDIR)

$(OSGENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(DEVSDIR)/%.handler: $(OBJDIR)/%_handler.o
	$(CC) $(ILDFLAGS) $< -o $@

$(OSGENDIR)/%.d: %.c
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -f$@ -p$(OSGENDIR)/ -- $(CFLAGS) -- $^

$(OBJDIR)/%.d: %.c
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -f$@ -p$(OBJDIR)/ -- $(CFLAGS) -- $^

include $(foreach f,$(FILES),$(OSGENDIR)/$(f).d)
include $(OBJDIR)/ram_handler.d $(OBJDIR)/nil_handler.d
