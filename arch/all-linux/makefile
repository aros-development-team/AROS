# $Id$

TOP=../..
COMPILE_KERNEL = yes # Must have a value, no matter which one

USER_INCLUDES=-I$(TOP)/rom/graphics -I$(TOP)/rom/intuition

include $(TOP)/config/make.cfg

FILES = graphics_driver intuition_driver

all: setup \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o)

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi

clean:
	-$(RM) $(OSGENDIR) *.err

config:
	@echo "Configuring for Linux..."

$(OSGENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(GUI_CCFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OSGENDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

ifneq ($(TARGET),clean)
-include $(foreach f,$(FILES),$(OSGENDIR)/$(f).d)
endif
