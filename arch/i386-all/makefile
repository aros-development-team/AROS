# $Id$
TOP=../..

include $(TOP)/config/make.cfg

SPECIAL_CFLAGS =$(SHARED_CFLAGS) -I/usr/include -I$(TOP)/exec
CLIBDIR = $(GENDIR)/compiler/clib
CFLAGS2 = $(SHARED_CFLAGS) $(CFLAGS)

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

CLIBFILES = \
	longjmp \
	setjmp

CFILES = $(wildcard $(foreach f,$(FILES),$(f).c))
DEPS = $(patsubst %.c,$(OSGENDIR)/%.d,$(CFILES))

all: setup $(MACHINE_H) machine.i \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o) \
	$(foreach f,$(CLIBFILES),$(CLIBDIR)/$(f).o)

setup :
	@for dir in $(OSGENDIR) $(CLIBDIR) ; do \
	    if [ ! -d $$dir ]; then $(MKDIR) $$dir ; else true ; fi ; \
	done
	@echo "CFLAGS=$(CFLAGS)"

$(MACHINE_H): machine.h
	cp $< $@

clean:
	$(RM) $(MACHINE_H) machine.i geninc

machine.i: geninc.c
	@echo "Creating geninc..."
	@$(CC) $(CFLAGS) -o geninc $<
	./geninc > machine.i

$(CLIBDIR)/%.o: %.s machine.i
	@$(ECHO) "Assembling $(CURDIR)/$<..."
	@$(AS) $(AFLAGS) -c $< -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OSGENDIR)/%.o: %.s machine.i
	@$(ECHO) "Assembling $(CURDIR)/$<..."
	@$(AS) $(AFLAGS) -c $< -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OSGENDIR)/%.o: %.c
	@$(ECHO) "Compiling $(CURDIR)/$<..."
	@$(CC) $(CFLAGS2) -c $< -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OSGENDIR)/%.d: %.c $(MACHINE_H)
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	@$(ECHO) "Makedepend $(CURDIR)/$<..."
	@$(MKDEPEND) -f- -p$(@D)/  -- $(CFLAGS) -- $< > $@

# Also check if the makefile itself must be remade
all : makefile

# Remake makefile
makefile : makefile.src $(TOP)/scripts/genmf.gawk \
	    $(TOP)/config/make.tmpl
	@$(ECHO) "Regenerating makefile..."
	@$(MV) $@ $@.bak
	@$(AWK) -f $(TOP)/scripts/genmf.gawk --assign TOP="$(TOP)" \
	    $< > $@

# Include these files but only if the current target is neither
# setup nor clean.
ifneq ("$(TARGET)","clean")
ifneq ("$(TARGET)","setup")
include $(DEPS)
endif
endif
