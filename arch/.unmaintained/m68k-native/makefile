# $Id$
TOP=..

include $(TOP)/make.cfg

SPECIAL_CFLAGS = -I $(TOP)/exec

OBJDIR=$(GENDIR)/m68k-native

all: setup  machine.i \
	    $(OSGENDIR)/supervisor.o $(OSGENDIR)/switch.o \
	    $(OSGENDIR)/dispatch.o $(OSGENDIR)/disable.o $(OSGENDIR)/enable.o \
	    $(OSGENDIR)/forbid.o $(OSGENDIR)/permit.o $(OSGENDIR)/preparecontext.o \
	    $(OSGENDIR)/special.o $(OSGENDIR)/semaphoreglue.o $(OSGENDIR)/exception.o \
	    $(OSGENDIR)/cachecleare.o $(OSGENDIR)/cacheclearu.o \
	    $(OSGENDIR)/cachecontrol.o $(OSGENDIR)/cachepostdma.o \
	    $(OSGENDIR)/cachepredma.o $(OSGENDIR)/setsr.o $(OSGENDIR)/getcc.o \
	    $(OSGENDIR)/superstate.o $(OSGENDIR)/userstate.o $(OSGENDIR)/stackswap.o

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi

clean:
	$(RM) $(OSGENDIR) $(LIBDIR)/libAmigaOS.a

machine.i: geninc.c
	$(CC) $(CFLAGS) -o geninc geninc.c
	geninc > machine.i

$(OSGENDIR)/%.o: %.s
	$(AS) $(AFLAGS) $< -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; else true ; fi

$(OSGENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; else true ; fi

$(OSGENDIR)/%.d: %.c
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -p$(OSGENDIR)/
