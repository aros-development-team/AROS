TOP=../..

USER_INCLUDES=-I$(TOP)/graphics -I$(TOP)/intuition

include $(TOP)/config/make.cfg

ifeq ($(FLAVOUR),native)
SUBDIRS = boot

FILES =

else
SUBDIRS =

FILES = graphics_driver intuition_driver
endif

all: setup subdirs \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o)

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi

clean:
	$(RM) $(OSGENDIR) *.err

config:
	@echo "Configuring for AmigaOS..."

$(OSGENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(GUI_CCFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OSGENDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

subdirs:
	@for dir in $(SUBDIRS) ; do \
	    echo "Making all in $$dir..." ; \
	    if ( cd $$dir ; \
		$(MAKE) $(MFLAGS) TOP="../../.." CURDIR="$(CURDIR)/$$dir"\
		all ) ; \
	    then echo -n ; else exit 1 ; fi ; \
	done

ifneq ($(FLAVOUR),native)
include $(foreach f,$(FILES),$(OSGENDIR)/$(f).d)
endif
