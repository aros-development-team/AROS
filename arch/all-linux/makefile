# $Id$

TOP=../..

#	-I./pthreads/include
USER_INCLUDES= \
	-I/usr/include -I$(TOP)/rom/graphics -I$(TOP)/rom/intuition \
	-I$(TOP)/rom/exec

include $(TOP)/config/make.cfg

FILES = graphics_driver intuition_driver \
	rawioinit rawmaygetchar rawputchar

all: setup \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o)

threads : $(LIBDIR)/libgthreads.a $(LIBDIR)/libmalloc.a
	cd pthreads/src ; make

$(LIBDIR)/libgthreads.a : pthreads/lib/libgthreads.a
	cp $? $@

$(LIBDIR)/libmalloc.a : pthreads/malloc/libmalloc.a
	cp $? $@

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi

clean:
	-$(RM) $(OSGENDIR) *.err

$(OSGENDIR)/%.o: %.c
	$(CC) $(SHARED_CFLAGS) $(CFLAGS) $(GUI_CCFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OSGENDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

ifeq ($(TARGET),clean)
NODEPS:=yes
endif
ifeq ($(TARGET),setup)
NODEPS:=yes
endif

ifndef NODEPS
include $(foreach f,$(FILES),$(OSGENDIR)/$(f).d)
endif
