# $Id$
TOP=../..

include $(TOP)/config/make.cfg

USER_INCLUDES = -I. -I./machines/$(KERNEL) -I$(TOP)/compiler/clib/include

OBJDIR = $(GENDIR)/compiler/vbcc
LIBS = -L$(LIBDIR) -larosc -lamiga -laros  #-lm
DEPLIBS = $(LIBDIR)/startup.o $(LIBDIR)/libarosc.a \
	$(LIBDIR)/libamiga.a $(LIBDIR)/libaros.a

BASIC_FILES = main vars declaration parse_expr type_expr ic machine \
	    statements preproc
CC_FILES = opt av rd regs flow cse cp loop alias
CCS_FILES = opts regss

CC_OBJS = $(foreach f,$(BASIC_FILES) $(CC_FILES),$(OBJDIR)/$(f).o)
CCS_OBJS = $(foreach f,$(BASIC_FILES) $(CCS_FILES),$(OBJDIR)/$(f).o)

EXES = $(EXEDIR)/vc $(EXEDIR)/vbcc$(KERNEL) $(EXEDIR)/vbccs$(KERNEL)

all : setup $(EXES)

local-gcc :
	-$(RM) $(EXES) $(OBJDIR)/*.o
	$(MAKE) $(MFLAGS) -f mf.local
	@if [ ! -d bin ]; then $(MKDIR) bin; else true ; fi
	mv vc vbcc$(KERNEL) vbccs$(KERNEL) bin/

local-vc :
	-$(RM) $(EXES) $(OBJDIR)/*.o
	$(MAKE) $(MFLAGS) -f mf.local CC="bin/vc +vc-local.config" APPCFLAGS="-v -I. -Imachines/i386" ILDFLAGS=

local-vc-test :
	-$(RM) $(EXES) $(OBJDIR)-vc/*.o
	$(MAKE) $(MFLAGS) -f mf.local OBJDIR="$(OBJDIR)-vc" CC="vc" APPCFLAGS="-v -I. -Imachines/i386" ILDFLAGS=

$(EXEDIR)/vc : vc.c $(DEPLIBS)
	$(CC) $(ILDFLAGS) $(LIBDIR)/startup.o $< -o $@ $(LIBS) 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(EXEDIR)/vbcci386 : $(CC_OBJS) $(DEPLIBS)
	$(CC) $(ILDFLAGS) $(LIBDIR)/startup.o $(CC_OBJS) -o $@ $(LIBS) 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(EXEDIR)/vbccsi386 : $(CCS_OBJS) $(DEPLIBS)
	$(CC) $(ILDFLAGS) $(LIBDIR)/startup.o $(CCS_OBJS) -o $@ $(LIBS) 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

setup :
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR); else true ; fi
	@if [ ! -d $(OBJDIR)-vc ]; then $(MKDIR) $(OBJDIR)-vc; else true ; fi
	@if [ ! -d $(EXEDIR) ]; then $(MKDIR) $(EXEDIR); else true ; fi

clean :
	$(RM) $(EXES) $(OBJDIR)

$(OBJDIR)/%.o: %.c
	$(CC) $(APPCFLAGS) -c $< -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OBJDIR)/machine.o: ./machines/$(KERNEL)/machine.c
	$(CC) $(APPCFLAGS) -c $< -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OBJDIR)/regss.o: regs.c
	$(CC) -DNO_OPTIMIZER $(APPCFLAGS) -c $< -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OBJDIR)/opts.o: opt.c
	$(CC) -DNO_OPTIMIZER $(APPCFLAGS) -c $< -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OBJDIR)/machine.d: ./machines/$(KERNEL)/machine.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	$(MKDEPEND) -f- -p$(@D)/ -- $(APPCFLAGS) -- $^ > $@

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	$(MKDEPEND) -f- -p$(@D)/ -- $(APPCFLAGS) -- $^ > $@

ifneq ($(TARGET),clean)
include $(foreach f,$(BASIC_FILES) $(CC_FILES),$(OBJDIR)/$(f).d)
endif
