# $Id$
TOP=../..

SPECIAL_CFLAGS = -D__AROS

include $(TOP)/config/make.cfg

INCLUDES = -I$(BINDIR)/include -I$(TOP)/amiga/include

#CC=$(PURIFY) $(SYS_CC)

OBJDIR = $(GENDIR)/workbench/c

STARTUP = $(LIBDIR)/startup.o
LIBS = -L$(LIBDIR)  -lamiga -larossupport -larosc 
DEPLIBS = $(LIBDIR)/libamiga.a $(LIBDIR)/libarossupport.a $(STARTUP) \
	$(LIBDIR)/libarosc.a 

#LIBS = -L$(LIBDIR) -larosc -lamiga -larossupport
#DEPLIBS = $(LIBDIR)/startup.o $(LIBDIR)/libarosc.a \
#	 $(LIBDIR)/libamiga.a $(LIBDIR)/libarossupport.a

FILES = \
	addbuffers \
	ask \
	assign \
	avail \
	cd \
	copy \
	date \
	delete \
	dir \
	echo \
	execute \
	liblist \
	list \
	makedir \
	mount \
	newshell \
	path \
	run \
	shell \
	tasklist \
	type \
	version \
	wait \
	why \
	rename

EXES = $(foreach f,$(FILES),$(EXEDIR)/$(f))
DEPS = $(foreach f,$(FILES),$(OBJDIR)/$(f).d)

all: setup $(EXES)

setup :
	@for dir in $(EXEDIR) $(OBJDIR) ; do \
	    if [ ! -d $$dir ]; then $(MKDIR) $$dir ; else true ; fi ; \
	done

clean:
	-$(RM) $(EXES)

$(EXEDIR)/%: $(OBJDIR)/%.o $(LIBDIR)/startup.o $(DEPLIBS)
	@$(ECHO) "Linking $@..."
	@$(CC) $(ILDFLAGS) $(STARTUP) $< -o $@ $(LIBS) 2>&1|tee $(@F)-x.err
	@if test ! -s $(@F)-x.err; then rm $(@F)-x.err ; else true ; fi

$(OBJDIR)/%.o: %.c
	@$(ECHO) "Compiling $(CURDIR)/$<..."
	@$(CC) $(APPCFLAGS) -c $< -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OBJDIR)/%.d: %.c
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
