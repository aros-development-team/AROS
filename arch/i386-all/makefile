# $Id$
TOP=../..

include $(TOP)/config/make.cfg

SPECIAL_CFLAGS =-I/usr/include -I$(TOP)/exec
CLIBDIR=$(GENDIR)/clib

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

CFILES = setjmp longjmp


all: setup $(MACHINE_H) machine.i \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o) \
	$(foreach f,$(CFILES),$(CLIBDIR)/$(f).o)

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi
	@if [ ! -d $(CLIBDIR) ]; then $(MKDIR) $(CLIBDIR) ; else true ; fi

$(MACHINE_H): machine.h
	cp $< $@

clean:
	$(RM) $(MACHINE_H) machine.i geninc

config:
	@echo "Configuring for a x86 CPU..."

machine.i: geninc.c
	$(CC) $(CFLAGS) -o geninc geninc.c
	./geninc > machine.i

$(CLIBDIR)/%.o: %.s machine.i
	$(AS) $(AFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OSGENDIR)/%.o: %.s machine.i
	$(AS) $(AFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OSGENDIR)/%.o: %.c
	$(CC) $(SHARED_CFLAGS) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OSGENDIR)/%.d: %.c $(TOP)/compiler/include/aros/machine.h
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

ifneq ("$(TARGET)","clean")
-include $(OSGENDIR)/geninc.d $(OSGENDIR)/preparecontext.d
endif
