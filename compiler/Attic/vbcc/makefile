# $Id$
TOP=../..

include $(TOP)/config/make.cfg

USER_INCLUDES = -I. -I./machines/$(KERNEL) -I$(TOP)/compiler/clib/include

OBJDIR = $(GENDIR)/compiler/vbcc

STARTUP = $(LIBDIR)/startup.o
LIBS = -L$(LIBDIR)  -lamiga -larossupport -larosc 
DEPLIBS = $(LIBDIR)/libamiga.a $(LIBDIR)/libarossupport.a $(STARTUP) \
	$(LIBDIR)/libarosc.a 

#LIBS = -L$(LIBDIR) -larosc -lamiga -larossupport  #-lm
#DEPLIBS = $(LIBDIR)/startup.o $(LIBDIR)/libarosc.a \
#	 $(LIBDIR)/libamiga.a $(LIBDIR)/libarossupport.a

BASIC_FILES = main vars declaration parse_expr type_expr ic machine \
	    statements preproc
CC_FILES = opt av rd regs flow cse cp loop alias
CCS_FILES = opts regss

CC_OBJS = $(foreach f,$(BASIC_FILES) $(CC_FILES),$(OBJDIR)/$(f).o)
CCS_OBJS = $(foreach f,$(BASIC_FILES) $(CCS_FILES),$(OBJDIR)/$(f).o)
DEPS = $(foreach f,$(BASIC_FILES) $(CC_FILES),$(OBJDIR)/$(f).d)

EXES = $(EXEDIR)/vc $(EXEDIR)/vbcc$(KERNEL) $(EXEDIR)/vbccs$(KERNEL)

all : setup $(EXES)

local-gcc :
	$(MAKE) $(MFLAGS) BINDIR=bin-gcc OBJDIR=obj-gcc -f mf.local

local-vc :
	$(MAKE) $(MFLAGS) -f mf.local \
	    BINDIR=bin-vcc OBJDIR=obj-vcc \
	    CC="bin-gcc/vc +vc-local.config" \
	    APPCFLAGS="-v -I. -Imachines/$(KERNEL)" \
	    ILDFLAGS=

local-vc-test :
	$(MAKE) $(MFLAGS) -f mf.local \
	    BINDIR=bin-vcc2 OBJDIR=obj-vcc2 \
	    CC="bin-vcc/vc +vc-localtest.config" \
	    APPCFLAGS="-v -I. -Imachines/$(KERNEL)" \
	    ILDFLAGS=

local-vc-test2 :
	$(MAKE) $(MFLAGS) -f mf.local \
	    BINDIR=bin-vcc3 OBJDIR=obj-vcc3 \
	    CC="bin-vcc2/vc +vc-localtest2.config" \
	    APPCFLAGS="-v -I. -Imachines/$(KERNEL)" \
	    ILDFLAGS=

$(EXEDIR)/vc : vc.c $(DEPLIBS)
	@$(ECHO) "Linking $@..."
	@$(CC) $(ILDFLAGS) $(STARTUP) $< -o $@ $(LIBS) 2>&1|tee $(@F)-x.err
	@if test ! -s $(@F)-x.err; then rm $(@F)-x.err ; else true ; fi

$(EXEDIR)/vbcc$(KERNEL) : $(CC_OBJS) $(DEPLIBS)
	@$(ECHO) "Linking $@..."
	@$(CC) $(ILDFLAGS) $(STARTUP) $(CC_OBJS) -o $@ $(LIBS) 2>&1|tee $(@F)-x.err
	@if test ! -s $(@F)-x.err; then rm $(@F)-x.err ; else true ; fi

$(EXEDIR)/vbccs$(KERNEL) : $(CCS_OBJS) $(DEPLIBS)
	@$(ECHO) "Linking $@..."
	@$(CC) $(ILDFLAGS) $(STARTUP) $(CCS_OBJS) -o $@ $(LIBS) 2>&1|tee $(@F)-x.err
	@if test ! -s $(@F)-x.err; then rm $(@F)-x.err ; else true ; fi

setup :
	@for dir in $(OBJDIR) $(OBJDIR)-vc $(EXEDIR) ; do \
	    if [ ! -d $$dir ]; then $(MKDIR) $$dir ; else true ; fi ; \
	done

clean :
	$(RM) $(EXES) $(OBJDIR)

$(OBJDIR)/%.o: %.c
	@$(ECHO) "Compiling $(CURDIR)/$<..."
	@$(CC) $(APPCFLAGS) -c $< -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OBJDIR)/machine.o: ./machines/$(KERNEL)/machine.c
	@$(ECHO) "Compiling $(CURDIR)/$<..."
	@$(CC) $(APPCFLAGS) -c $< -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

APPCFLAGS2 = -DNO_OPTIMIZER $(APPCFLAGS)

$(OBJDIR)/regss.o: regs.c
	@$(ECHO) "Compiling $(CURDIR)/$<..."
	@$(CC) $(APPCFLAGS2) -c $< -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OBJDIR)/opts.o: opt.c
	@$(ECHO) "Compiling $(CURDIR)/$<..."
	@$(CC) $(APPCFLAGS2) -c $< -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OBJDIR)/machine.d: ./machines/$(KERNEL)/machine.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	@$(ECHO) "Makedepend $(CURDIR)/$<..."
	@$(MKDEPEND) -f- -p$(@D)/  -- $(APPCFLAGS) -- $< > $@

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	@$(ECHO) "Makedepend $(CURDIR)/$<..."
	@$(MKDEPEND) -f- -p$(@D)/  -- $(APPCFLAGS) -- $< > $@

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
