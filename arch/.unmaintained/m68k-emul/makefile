# $Id$
TOP=../..

include $(TOP)/config/make.cfg

SPECIAL_CFLAGS = -I $(TOP)/exec

MACHINE_H = $(INCDIR)/aros/machine.h

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

all: setup $(MACHINE_H) machine.i \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o)

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi

$(MACHINE_H): machine.h
	$(CP) $< $@

clean:
	$(RM) $(MACHINE_H) machine.i geninc

machine.i: geninc.c
	$(CC) $(CFLAGS) -o geninc geninc.c
	./geninc > machine.i

$(OSGENDIR)/%.o: %.s machine.i
	$(AS) $(AFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OSGENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OSGENDIR)/%.d: %.c $(MACHINE_H)
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

include $(OSGENDIR)/geninc.d
include $(OSGENDIR)/preparecontext.d
include $(OSGENDIR)/initcore.d
