TOP=..

SPECIAL_CFLAGS = -D__AROS

include $(TOP)/make.cfg

#CC=$(PURIFY) $(SYS_CC)

OBJDIR = $(GENDIR)/exe
LIBS = -L$(LIBDIR) -lamiga -laros
DEPLIBS = $(LIBDIR)/libamiga.a $(LIBDIR)/libaros.a

FILES = helloworld \
	newshell \
	type \
	echo \
	cd \
	avail \
	list \
	librarytest \
	run \
	execute \
	path \
	shell \
	dir \
	wait \
	demowin \
	assign \
	delete \
	makedir \
	mount \
	simpleimage \
	intuitext

EXES = $(foreach f,$(FILES),$(EXEDIR)/$(f))

all: setup $(EXES) $(EXEDIR)/forkbomb $(OBJDIR)/startup.o

setup :
	@if [ ! -d $(EXEDIR) ]; then $(MKDIR) $(EXEDIR) ; fi
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; fi

clean:
	$(RM) $(EXES)

$(EXEDIR)/forkbomb : forkbomb
	$(CP) $< $@

$(EXEDIR)/dir: $(OBJDIR)/dir.o $(DEPLIBS)
	$(CC) $(ILDFLAGS) $(OBJDIR)/startup.o $< -o $@ $(LIBS)

$(EXEDIR)/librarytest: $(OBJDIR)/librarytest.o $(DEPLIBS)
	$(CC) $(ILDFLAGS) $(OBJDIR)/startup.o $< -o $@ $(LIBS)

$(OBJDIR)/librarytest.o: librarytest.c $(TOP)/libs/dummylib_gcc.h
	$(CC) $(CFLAGS) -I $(TOP)/libs -c $< -o $@

$(EXEDIR)/%: $(OBJDIR)/%.o $(OBJDIR)/startup.o $(DEPLIBS)
	$(CC) $(ILDFLAGS) $(OBJDIR)/startup.o $< -o $@ $(LIBS)

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

include $(foreach f,$(FILES),$(OBJDIR)/$(f).d) $(OBJDIR)/startup.d

