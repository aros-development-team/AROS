# $Id$
TOP=../..

SPECIAL_CFLAGS = -D__AROS

include $(TOP)/config/make.cfg

#CC=$(PURIFY) $(SYS_CC)
EXEDIR=$(BINDIR)/Demos

OBJDIR = $(GENDIR)/demos
LIBS = -L$(LIBDIR) -lamiga -laros -larosc
DEPLIBS = $(LIBDIR)/libamiga.a $(LIBDIR)/libaros.a $(LIBDIR)/startup.o

FILES = helloworld \
	librarytest \
	demowin \
	simpleimage \
	intuitext \
	compleximage \
	shadowborder \
	readicon

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
	$(RM) $(EXES)

$(EXEDIR)/forkbomb : forkbomb
	$(CP) $< $@

$(EXEDIR)/librarytest: $(OBJDIR)/librarytest.o $(DEPLIBS)
	$(CC) $(ILDFLAGS) $(LIBDIR)/startup.o $< -o $@ $(LIBS)

$(OBJDIR)/librarytest.o: librarytest.c $(TOP)/workbench/libs/dummylib_gcc.h
	$(CC) $(APPCFLAGS) -I$(TOP)/workbench/libs -c $< -o $@

$(EXEDIR)/%: $(OBJDIR)/%.o $(DEPLIBS)
	$(CC) $(ILDFLAGS) $(LIBDIR)/startup.o $< -o $@ $(LIBS)

$(OBJDIR)/%.o: %.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -f$@ -p$(OBJDIR)/ -- $(APPCFLAGS) -- $^

$(OBJDIR)/librarytest.d: librarytest.c
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -f$@ -p$(OBJDIR)/ -I$(TOP)/libs -- $(APPCFLAGS) -- $^

include $(foreach f,$(FILES),$(OBJDIR)/$(f).d)

