# $Id$
TOP=../..

SPECIAL_CFLAGS = -D__AROS

include $(TOP)/config/make.cfg

INCLUDES = -I$(BINDIR)/include -I$(TOP)/amiga/include

#CC=$(PURIFY) $(SYS_CC)

OBJDIR = $(GENDIR)/exe
LIBS = -L$(LIBDIR) -larosc -lamiga -laros
DEPLIBS = $(LIBDIR)/startup.o $(LIBDIR)/libarosc.a \
	$(LIBDIR)/libamiga.a $(LIBDIR)/libaros.a

FILES = \
	newshell \
	type \
	echo \
	cd \
	avail \
	list \
	run \
	execute \
	path \
	shell \
	dir \
	wait \
	assign \
	delete \
	makedir \
	mount \
	version \
	copy \
	tasklist

EXES = $(foreach f,$(FILES),$(EXEDIR)/$(f))

all: setup $(EXES)

setup :
	@if [ ! -d $(EXEDIR) ]; then $(MKDIR) $(EXEDIR) ; else true ; fi
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi

clean:
	-$(RM) $(EXES)

$(EXEDIR)/%: $(OBJDIR)/%.o $(LIBDIR)/startup.o $(DEPLIBS)
	$(CC) $(ILDFLAGS) $(LIBDIR)/startup.o $< -o $@ $(LIBS)

$(OBJDIR)/%.o: %.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

ifneq ($(TARGET),clean)
include $(foreach f,$(FILES),$(OBJDIR)/$(f).d)
endif
