# $Id$
TOP=../..

include $(TOP)/config/make.cfg

SPECIAL_CFLAGS = -I $(TOP)/exec

FILES = \
	supervisor \
	switch \
	dispatch \
	disable \
	enable \
	forbid \
	permit \
	preparecontext \
	special \
	semaphoreglue \
	exception \
	cachecleare \
	cacheclearu \
	cachecontrol \
	cachepostdma \
	cachepredma \
	setsr \
	getcc \
	superstate \
	userstate \
	stackswap \
	runprocess \
	initcore

all: setup $(TOP)/apps/compiler/include/aros/machine.h machine.i \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o)

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi

$(TOP)/apps/compiler/include/aros/machine.h: machine.h
	cp machine.h $(TOP)/apps/compiler/include/aros

clean:
	$(RM) $(TOP)/apps/compiler/include/aros/machine.h

config:
	@echo "Configuring for a x86 CPU..."

machine.i: geninc.c
	$(CC) $(CFLAGS) -o geninc geninc.c
	./geninc > machine.i

$(OSGENDIR)/%.o: %.s machine.i
	$(AS) $(AFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OSGENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OSGENDIR)/%.d: %.c $(TOP)/apps/compiler/include/aros/machine.h
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -f$@ -p$(OSGENDIR)/ -- $(CFLAGS) -- $^

include $(OSGENDIR)/geninc.d
include $(OSGENDIR)/preparecontext.d

