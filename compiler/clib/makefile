# $Id$
TOP=..

USER_INCLUDES=-Iinclude

include $(TOP)/config/make.cfg

OBJDIR=$(GENDIR)/clib

FILES = ctype printf strcmp stricmp strlen strcpy qsort \
	strdup malloc free calloc realloc memcpy bcopy \
	bzero memset

all: setup $(OBJDIR)/links $(LIBDIR)/libarosc.a

setup:
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi

$(OBJDIR)/links:
	$(TOP)/scripts/makelinks include $(BINDIR)/include "*.h"
	touch $(OBJDIR)/links

clean:
	$(RM) $(OBJDIR) *.err $(LIBDIR)/libc.a

$(OBJDIR)/%.o: %.c
	$(SYS_CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err ; else true ; fi

$(LIBDIR)/libarosc.a: $(foreach f,$(FILES),$(OBJDIR)/$(f).o)
	$(AR) $@ $?
	$(RANLIB) $@

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -p$(OBJDIR)/

include $(foreach f,$(FILES),$(OBJDIR)/$(f).d)

