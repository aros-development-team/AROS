# $Id$
TOP=..

include $(TOP)/config/make.cfg

OBJDIR = $(GENDIR)/devs
LIBS = -L$(LIBDIR) -larosc -lamiga -laros

DEVS = $(DEVSDIR)/ram.handler \
	$(DEVSDIR)/nil.handler \
	$(DEVSDIR)/ffs.handler \
	$(DEVSDIR)/fdsk.device

FILES = console cdinputhandler rawkeyconvert

all: setup \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o) \
	$(DEVS) \
	$(DEVSDIR)/Mountlist

setup:
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi
	@if [ ! -d $(DEVSDIR) ]; then $(MKDIR) $(DEVSDIR) ; else true ; fi

clean:
	$(RM) $(OSGENDIR)
	$(RM) $(DEVSDIR)

$(DEVSDIR)/Mountlist : Mountlist
	$(CP) $< $@

$(OSGENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(DEVSDIR)/%.handler: $(OBJDIR)/%_handler.o
	$(CC) $(ILDFLAGS) $< -o $@ $(LIBS)

$(DEVSDIR)/%.device: $(OBJDIR)/%_device.o
	$(CC) $(ILDFLAGS) $< -o $@ $(LIBS)

$(OSGENDIR)/%.d: %.c
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -p$(OSGENDIR)/

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -p$(OBJDIR)/

include $(foreach f,$(FILES),$(OSGENDIR)/$(f).d)
include $(OBJDIR)/ram_handler.d $(OBJDIR)/nil_handler.d
