# $Id$
TOP=../..

include $(TOP)/config/make.cfg

SPECIAL_CFLAGS =-I/usr/include -I$(TOP)/exec
CLIBDIR=$(GENDIR)/clib

MACHINE_H = $(INCDIR)/aros/machine.h

FILES = \
	cache \
	disable \
	dispatch \
	enable \
	exception \
	forbid \
	getcc \
	initcore \
	permit \
	preparecontext \
	runprocess \
	semaphoreglue \
	setsr \
	special \
	stackswap \
	superstate \
	supervisor \
	switch \
	userstate

CFILES = $(wildcard $(foreach f,$(FILES),$(f).c))
DEPS = $(patsubst %.c,$(OSGENDIR)/%.d,$(CFILES))

all: setup $(MACHINE_H) machine.i \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o)

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi
	@if [ ! -d $(CLIBDIR) ]; then $(MKDIR) $(CLIBDIR) ; else true ; fi

$(MACHINE_H): machine.h
	cp $< $@

clean:
	$(RM) $(MACHINE_H) machine.i geninc

machine.i: geninc.c
	$(CC) $(CFLAGS) -o geninc $<
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

$(OSGENDIR)/%.d: %.c $(MACHINE_H)
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

ifneq ("$(TARGET)","clean")
ifneq ("$(TARGET)","setup")
include $(DEPS)
endif
endif
