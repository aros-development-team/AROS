# $Id$
TOP=../../..
COMPILE_KERNEL = yes # Must have a value, no matter which one

include $(TOP)/config/make.cfg

OBJDIR=$(GENDIR)/filesys

all: setup $(OBJDIR)/emul_handler.o

setup:
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi

clean:
	-$(RM) $(OBJDIR)

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

ifneq ($(TARGET),clean)
include $(OBJDIR)/emul_handler.d
endif
