# $Id$
TOP=../..

# Use OS's own includes over the ones from AROS
SPECIAL_CFLAGS=-I/usr/include

include $(TOP)/config/make.cfg

FILES = alerthook

OBJS = $(foreach f,$(FILES) $(FUNCTIONS),$(OSGENDIR)/$(f).o)

# ***** Native ***************
ifeq ($(FLAVOUR),native)

all: setup $(OSMODDIR)/alert.hook

$(OSMODDIR)/alert.hook: $(OBJS)
	$(CC) $(ILDFLAGS) $(OBJS) -Xlinker -M -o $@ 2>&1|tee alert.map
	@if test ! -s alert.map ; then rm alert.map ; else true ; fi
	@strip $@

# ***** Not Native ***************
else

all: setup $(OBJS)

# ***** END ***************
endif

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi
	@if [ ! -d $(OSMODDIR) ]; then $(MKDIR) $(OSMODDIR) ; else true ; fi

clean:

$(OSGENDIR)/%.o: %.c
	$(CC) $(SHARED_CFLAGS) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OSGENDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

ifneq ($(TARGET),clean)
include $(foreach f,$(FILES) $(FUNCTIONS),$(OSGENDIR)/$(f).d)
endif
