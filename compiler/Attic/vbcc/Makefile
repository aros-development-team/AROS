# $Id$
TOP=../../..

include $(TOP)/config/make.cfg

USER_INCLUDES = -I. -I./machines/$(KERNEL) -I$(TOP)/compiler/clib/include

OBJDIR = $(GENDIR)/compiler/vbcc
LIBS = -L$(LIBDIR) -larosc -lamiga -laros
DEPLIBS = $(LIBDIR)/startup.o $(LIBDIR)/libarosc.a \
	$(LIBDIR)/libamiga.a $(LIBDIR)/libaros.a

BASIC_FILES = main vars declaration parse_expr type_expr ic machine \
	    statements preproc
CC_FILES = opt av rd regs flow cse cp loop alias
CCS_FILES = opts regss

CC_OBJS = $(foreach f,$(BASIC_FILES) $(CC_FILES),$(OBJDIR)/$(f).o)
CCS_OBJS = $(foreach f,$(BASIC_FILES) $(CCS_FILES),$(OBJDIR)/$(f).o)

EXES = $(EXEDIR)/vc $(EXEDIR)/vbcc $(EXEDIR)/vbccs

all : setup $(EXES)

$(EXEDIR)/vc : vc.c $(LIBDIR)/startup.o $(DEPLIBS)
	$(CC) $(ILDFLAGS) $(LIBDIR)/startup.o $< -o $@ $(LIBS)

$(EXEDIR)/vbcc : $(CC_OBJS) $(LIBDIR)/startup.o $(DEPLIBS)
	$(CC) $(ILDFLAGS) $(LIBDIR)/startup.o $(CC_OBJS) -o $@ $(LIBS) #-lm

$(EXEDIR)/vbccs : $(CCS_OBJS) $(LIBDIR)/startup.o $(DEPLIBS)
	$(CC) $(ILDFLAGS) $(LIBDIR)/startup.o $(CCS_OBJS) -o $@ $(LIBS) #-lm

setup :
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR); else true ; fi
	@if [ ! -d $(EXEDIR) ]; then $(MKDIR) $(EXEDIR); else true ; fi

clean :
	$(RM) $(EXES) $(OBJDIR)

$(OBJDIR)/%.o: %.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/machine.o: ./machines/$(KERNEL)/machine.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/regss.o: regs.c
	$(CC) -DNO_OPTIMIZER $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/opts.o: opt.c
	$(CC) -DNO_OPTIMIZER $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/machine.d: ./machines/$(KERNEL)/machine.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	$(MKDEPEND) -f- -p$(@D)/ -- $(APPCFLAGS) -- $^ > $@

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	$(MKDEPEND) -f- -p$(@D)/ -- $(APPCFLAGS) -- $^ > $@

ifneq ($(TARGET),clean)
include $(foreach f,$(BASIC_FILES) $(CC_FILES),$(OBJDIR)/$(f).d)
endif
