TOP=..

include $(TOP)/make.cfg

#CC=$(PURIFY) $(SYS_CC)

OBJDIR = $(GENDIR)/exe
LIBS = -L$(LIBDIR) -laros

EXES = $(EXEDIR)/helloworld \
	$(EXEDIR)/newshell \
	$(EXEDIR)/type \
	$(EXEDIR)/echo \
	$(EXEDIR)/cd \
	$(EXEDIR)/avail \
	$(EXEDIR)/list \
	$(EXEDIR)/librarytest \
	$(EXEDIR)/run \
	$(EXEDIR)/execute \
	$(EXEDIR)/path \
	$(EXEDIR)/shell \
	$(EXEDIR)/dir \
	$(EXEDIR)/wait \
	$(EXEDIR)/forkbomb \
	$(EXEDIR)/demowin \
	$(EXEDIR)/assign \
	$(EXEDIR)/delete \
	$(EXEDIR)/makedir \
	$(EXEDIR)/mount

all: setup $(EXES)

setup :
	@if [ ! -d $(EXEDIR) ]; then $(MKDIR) $(EXEDIR) ; fi
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; fi

clean:
	$(RM) $(EXES)

$(EXEDIR)/forkbomb : forkbomb
	$(CP) $< $@

$(EXEDIR)/dir: $(OBJDIR)/dir.o $(LIBDIR)/libaros.a
	$(CC) $(ILDFLAGS) $^ -o $@ $(LIBS)

$(EXEDIR)/librarytest: $(OBJDIR)/librarytest.o
	$(CC) $(ILDFLAGS) $^ -o $@ $(LIBS)

$(OBJDIR)/librarytest.o: librarytest.c $(TOP)/libs/dummylib_gcc.h
	$(CC) $(CFLAGS) -I $(TOP)/libs -c $< -o $@

$(EXEDIR)/%: $(OBJDIR)/%.o
	$(CC) $(ILDFLAGS) $< -o $@ $(LIBS)

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

