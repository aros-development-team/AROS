###############################################################
###############################################################
######### THIS IS A GENERATED FILE. DO NOT EDIT ###############
###############################################################
###############################################################
TOP=../../..

include $(TOP)/config/make.cfg

FILES = readbattclock

# You don't need to change anything after here unless you add new
# directories to the above.

SPECIAL_CFLAGS= $(SHARED_CFLAGS) -I/usr/include -I$(TOP)/rom/battclock/ -I..
CFLAGS2 = $(SHARED_CFLAGS) $(CFLAGS)

CFILES = $(foreach f,$(FILES),$(f).c)
DEPS = $(patsubst %.c,$(OSGENDIR)/battclock/%.d,$(CFILES))

all: setup $(foreach f,$(FILES), $(OSGENDIR)/battclock/$(f).o)

setup:
	@for dir in $(OSGENDIR) $(OSGENDIR)/battclock ; do \
	    if [ ! -d $$dir ]; then $(MKDIR) $$dir ; else true ; fi ; \
	done
	$(foreach f,rom/battclock/readbattclock,$(shell echo >> $(GENDIR)/config.files add $(f)))
	$(foreach f,rom/battclock/readbattclock,$(shell echo >> $(GENDIR)/config.files not $(f)))
	@echo "CFLAGS= $(CFLAGS2)"

clean::
	$(RM) $(TESTS)

$(OSGENDIR)/battclock/%.o : %.s
	@$(ECHO) "Assembling $(CURDIR)/$<..."
	@$(AS) $(AFLAGS) -c $< -o $@ 2>&1|tee $*.err
	@if test -s $*.err; then $(TOUCH) $(TOP)/errors ; else $(RM) $*.err ; fi

$(OSGENDIR)/battclock/%.o : %.c
	@$(ECHO) "Compiling $(CURDIR)/$<..."
	@$(CC) $(CFLAGS2) -c $< -o $@ 2>&1|tee $*.err
	@if test -s $*.err; then $(TOUCH) $(TOP)/errors ; else $(RM) $*.err ; fi

$(OSGENDIR)/battclock/%.d : %.c $(MACHINE_H)
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	@$(ECHO) "Makedepend $(CURDIR)/$<..."
	@$(MKDEPEND) -f- -p$(@D)/  -- $(CFLAGS) -- $< > $@

# Check if the makefile itself must be remade
clean ::
	$(RM) makefile makefile.bak

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
