TOP=..

include $(TOP)/make.cfg

#CC=$(PURIFY) $(SYS_CC)

OBJDIR = $(GENDIR)/exe
LIBS = -L$(LIBDIR) -laros

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
	mount

EXES = $(foreach f,$(FILES),$(EXEDIR)/$(f))

all: setup $(EXES) $(EXEDIR)/forkbomb

setup :
	@if [ ! -d $(EXEDIR) ]; then $(MKDIR) $(EXEDIR) ; fi
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; fi

clean:
	$(RM) $(EXES)

$(EXEDIR)/forkbomb : forkbomb
	$(CP) $< $@

$(EXEDIR)/dir: $(OBJDIR)/dir.o $(LIBDIR)/libaros.a
	$(CC) $(ILDFLAGS) $< -o $@ $(LIBS)

$(EXEDIR)/librarytest: $(OBJDIR)/librarytest.o
	$(CC) $(ILDFLAGS) $< -o $@ $(LIBS)

$(OBJDIR)/librarytest.o: librarytest.c $(TOP)/libs/dummylib_gcc.h
	$(CC) $(CFLAGS) -I $(TOP)/libs -c $< -o $@

$(EXEDIR)/%: $(OBJDIR)/%.o
	$(CC) $(ILDFLAGS) $< -o $@ $(LIBS)

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.d: %.c
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -f$@ -p$(OBJDIR)/ -- $(CFLAGS) -- $^

$(OBJDIR)/librarytest.d: librarytest.c
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -f$@ -p$(OBJDIR)/ -- $(CFLAGS) -- $^

include $(foreach f,$(FILES),$(OBJDIR)/$(f).d)

