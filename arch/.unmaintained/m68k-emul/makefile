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
      runprocess

all: setup $(TOP)/include/aros/machine.h machine.i \
      $(foreach f,$(FILES),$(OSGENDIR)/$(f).o)

setup :
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi

clean:
	$(RM) $(TOP)/include/aros/machine.h

config:
      @echo "Configuring for a m68k CPU..."

machine.i: geninc.c
      $(CC) $(CFLAGS) -o geninc geninc.c
      ./geninc > machine.i

$(OSGENDIR)/%.o: %.s machine.i
      $(AS) $(AFLAGS) $< -c -o $@ 2>&1|tee $*.err
      if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OSGENDIR)/%.o: %.c
      $(CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
      if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OSGENDIR)/%.d: %.c $(TOP)/include/aros/machine.h
      @if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi
      @$(RM) $@
      @touch $@
      $(MKDEPEND) -p$(OSGENDIR)/

include $(OSGENDIR)/geninc.d

