TOP=..

SPECIAL_CFLAGS = -D__AROS

include $(TOP)/make.cfg

#CC=$(PURIFY) $(SYS_CC)
EXEDIR=$(BINDIR)/Demos

OBJDIR = $(GENDIR)/demos
LIBS = -L$(LIBDIR) -lamiga -laros
DEPLIBS = $(LIBDIR)/libamiga.a $(LIBDIR)/libaros.a $(LIBDIR)/startup.o

FILES = helloworld \
	librarytest \
	demowin \
	simpleimage \
	intuitext \
	compleximage \
	shadowborder

IMAGES = ArrowUp ArrowDown ArrowLeft ArrowRight ImageButton

EXES = $(foreach f,$(FILES),$(EXEDIR)/$(f))

all: setup data $(EXES) $(EXEDIR)/forkbomb

data: $(foreach f,$(IMAGES),images/$(f)0.h images/$(f)1.h)

images/%.h : images/%.dat images/datfilt.awk
	cd images ; gawk -f datfilt.awk $(<F)

setup :
	@if [ ! -d $(EXEDIR) ]; then $(MKDIR) $(EXEDIR) ; fi
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; fi

clean:
	$(RM) $(EXES)

$(EXEDIR)/forkbomb : forkbomb
	$(CP) $< $@

$(EXEDIR)/librarytest: $(OBJDIR)/librarytest.o $(DEPLIBS)
	$(CC) $(ILDFLAGS) $(LIBDIR)/startup.o $< -o $@ $(LIBS)

$(OBJDIR)/librarytest.o: librarytest.c $(TOP)/libs/dummylib_gcc.h
	$(CC) $(CFLAGS) -I $(TOP)/libs -c $< -o $@

$(EXEDIR)/%: $(OBJDIR)/%.o $(DEPLIBS)
	$(CC) $(ILDFLAGS) $(LIBDIR)/startup.o $< -o $@ $(LIBS)

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; fi
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -f$@ -p$(OBJDIR)/ -- $(CFLAGS) -- $^

$(OBJDIR)/librarytest.d: librarytest.c
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; fi
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -f$@ -p$(OBJDIR)/ -- $(CFLAGS) -I $(TOP)/libs -- $^

include $(foreach f,$(FILES),$(OBJDIR)/$(f).d)

