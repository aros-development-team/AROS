# $Id$
TOP=../../..

include $(TOP)/config/make.cfg

OBJDIR=$(GENDIR)/aros

FILES = calcchecksum kprintf nastyfreemem \
	    purify removeslist debugmem \
	    _kprintf hexdump

all: setup $(LIBDIR)/libaros.a

setup :
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi

clean:
	$(RM) $(OBJDIR) *.err $(LIBDIR)/libaros.a

$(OBJDIR)/%.o: %.c
	$(SYS_CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err; else true ; fi

$(OBJDIR)/rt_stubs.o: rt_stubs.s
	$(SYS_AS) $(AFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err; else true ; fi

$(LIBDIR)/libaros.a: $(foreach f,$(FILES),$(OBJDIR)/$(f).o)
	$(AR) $@ $?
	$(RANLIB) $@

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -f$@ -p$(OBJDIR)/ -- $(CFLAGS) -- $^

include $(foreach f,$(FILES),$(OBJDIR)/$(f).d)

