# $Id$
TOP=../..

SPECIAL_CFLAGS = -D__AROS -I$(TOP)/workbench/libs

include $(TOP)/config/make.cfg

#CC=$(PURIFY) $(SYS_CC)
EXEDIR=$(BINDIR)/Demos

OBJDIR = $(GENDIR)/workbench/demos
LIBS = -L$(LIBDIR) -lamiga -larossupport -larosc
DEPLIBS = $(LIBDIR)/libamiga.a $(LIBDIR)/libarossupport.a $(LIBDIR)/startup.o \
	$(LIBDIR)/libarosc.a

FILES = helloworld \
	librarytest \
	demowin \
	simpleimage \
	intuitext \
	compleximage \
	shadowborder \
	readicon \
	sift \
	regiontest

IMAGES = ArrowUp ArrowDown ArrowLeft ArrowRight ImageButton
IMAGEFILES = $(foreach f,$(IMAGES),images/$(f)0.h images/$(f)1.h)

EXES = $(foreach f,$(FILES),$(EXEDIR)/$(f))

all: setup data $(EXES) $(EXEDIR)/forkbomb

data: $(IMAGEFILES)

images/%.h : images/%.dat images/datfilt.awk
	cd images ; gawk -f datfilt.awk $(<F)

setup :
	@if [ ! -d $(EXEDIR) ]; then $(MKDIR) $(EXEDIR) ; else true ; fi
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi

clean:
	-$(RM) $(EXES)

$(EXEDIR)/forkbomb : forkbomb
	$(CP) $< $@

$(EXEDIR)/librarytest: $(OBJDIR)/librarytest.o $(DEPLIBS)
	$(CC) $(ILDFLAGS) -L../libs $(LIBDIR)/startup.o $< -o $@ $(LIBS) 2>&1|tee librarytest-x.err
	@if test ! -s librarytest-x.err; then rm librarytest-x.err ; else true ; fi

$(OBJDIR)/librarytest.o: librarytest.c $(TOP)/workbench/libs/dummylib_gcc.h
	$(CC) $(APPCFLAGS) -c $< -o $@ 2>&1|tee librarytest.err
	@if test ! -s librarytest.err; then rm librarytest.err ; else true ; fi

$(EXEDIR)/%: $(OBJDIR)/%.o $(DEPLIBS)
	$(CC) $(ILDFLAGS) $(LIBDIR)/startup.o $< -o $@ $(LIBS) 2>&1|tee $*-x.err
	@if test ! -s $*-x.err; then rm $*-x.err ; else true ; fi

$(OBJDIR)/%.o: %.c
	$(CC) $(APPCFLAGS) -c $< -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

$(OBJDIR)/librarytest.d: librarytest.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

ifneq ($(TARGET),clean)
include $(foreach f,$(FILES),$(OBJDIR)/$(f).d)
endif
