TOP=..

include $(TOP)/make.cfg

OBJDIR=$(GENDIR)/aros

FILES = calcchecksum strcmp \
	    strlen strcpy qsort \
	    kprintf nastyfreemem \
	    rt purify strdup \
	    stricmp removeslist

all: setup $(LIBDIR)/libaros.a

setup :
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; fi

clean:
	$(RM) $(OBJDIR) *.err $(LIBDIR)/libaros.a

$(OBJDIR)/%.o: %.c
	$(SYS_CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

$(OBJDIR)/rt_stubs.o: rt_stubs.s
	$(SYS_AS) $(AFLAGS) $< -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

$(LIBDIR)/libaros.a: $(foreach f,$(FILES),$(OBJDIR)/$(f).o) \
	    $(OBJDIR)/rt_stubs.o
	$(AR) $@ $?
	$(RANLIB) $@

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; fi
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -f$@ -p$(OBJDIR)/ -- $(CFLAGS) -- $^

include $(foreach f,$(FILES),$(OBJDIR)/$(f).d)

