TOP=..

include $(TOP)/make.cfg

SPECIAL_CFLAGS = -I $(TOP)/exec

all: setup $(OSGENDIR)/init.o $(OSGENDIR)/supervisor.o $(OSGENDIR)/switch.o \
	    $(OSGENDIR)/dispatch.o $(OSGENDIR)/disable.o $(OSGENDIR)/enable.o \
	    $(OSGENDIR)/forbid.o $(OSGENDIR)/permit.o $(OSGENDIR)/preparecontext.o \
	    $(OSGENDIR)/special.o $(OSGENDIR)/semaphoreglue.o $(OSGENDIR)/exception.o \
	    $(OSGENDIR)/cachecleare.o $(OSGENDIR)/cacheclearu.o \
	    $(OSGENDIR)/cachecontrol.o $(OSGENDIR)/cachepostdma.o \
	    $(OSGENDIR)/cachepredma.o $(OSGENDIR)/setsr.o $(OSGENDIR)/getcc.o \
	    $(OSGENDIR)/superstate.o $(OSGENDIR)/userstate.o $(OSGENDIR)/stackswap.o \
	    $(OSGENDIR)/runprocess.o

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; fi

clean:
	$(RM) $(OSGENDIR) $(LIBDIR)/libkernel.a

$(OSGENDIR)/%.o: %.s
	$(AS) $(AFLAGS) $< -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

$(OSGENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

$(OSGENDIR)/%.d: %.c
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; fi
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -f$@ -p$(OSGENDIR)/ -- $(CFLAGS) -- $^

include $(OSGENDIR)/init.d
