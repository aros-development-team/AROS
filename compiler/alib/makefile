# $Id$
TOP=..

SPECIAL_CFLAGS = -D__AROS

include $(TOP)/config/make.cfg

OBJDIR=$(GENDIR)/alib

FILES=alib_util newobject setattrs openwindowtags openscreentags \
	dogadgetmethod domethod dosupermethod readbyte readword \
	readlong readfloat readdouble readstring writebyte writeword \
	writelong writefloat writedouble writestring

all: setup $(LIBDIR)/libamiga.a $(LIBDIR)/startup.o

setup :
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi

clean:
	$(RM) $(OBJDIR) *.err $(LIBDIR)/libamiga.a

$(OBJDIR)/%.o: %.c
	$(SYS_CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(LIBDIR)/%.o: %.c
	$(SYS_CC) $(CFLAGS) -c $< -o $@

$(LIBDIR)/libamiga.a: $(foreach f,$(FILES),$(OBJDIR)/$(f).o)
	$(AR) $@ $?
	$(RANLIB) $@

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -f$@ -p$(OBJDIR)/ -- $(CFLAGS) -- $^

include $(foreach f,$(FILES),$(OBJDIR)/$(f).d) $(OBJDIR)/startup.d
