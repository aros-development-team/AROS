# $Id$
TOP=../..

include $(TOP)/config/make.cfg

OBJDIR = $(GENDIR)/devs
LIBS = -L$(LIBDIR) -larosc -lamiga -laros

DEVS = $(DEVSDIR)/ram.handler \
	$(DEVSDIR)/nil.handler \
	$(DEVSDIR)/ffs.handler \
	$(DEVSDIR)/fdsk.device

all: setup \
	$(DEVS) \
	$(DEVSDIR)/Mountlist

setup:
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi
	@if [ ! -d $(DEVSDIR) ]; then $(MKDIR) $(DEVSDIR) ; else true ; fi

clean:
	$(RM) $(DEVSDIR)

$(DEVSDIR)/Mountlist : Mountlist
	$(CP) $< $@

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(DEVSDIR)/%.handler: $(OBJDIR)/%_handler.o
	$(CC) $(ILDFLAGS) $< -o $@ $(LIBS)

$(DEVSDIR)/%.device: $(OBJDIR)/%_device.o
	$(CC) $(ILDFLAGS) $< -o $@ $(LIBS)

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -f$@ -p$(OBJDIR)/ -- $(CFLAGS) -- $^

include $(OBJDIR)/ram_handler.d $(OBJDIR)/nil_handler.d
