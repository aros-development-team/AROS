TOP=..

include $(TOP)/make.cfg

OBJDIR = $(GENDIR)/exe

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
	$(EXEDIR)/forkbomb

all: setup $(EXES)

setup :
	@if [ ! -d $(EXEDIR) ]; then $(MKDIR) $(EXEDIR) ; fi
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; fi

clean:
	$(RM) $(EXES)

$(EXEDIR)/forkbomb : forkbomb
	$(CP) $< $@

$(EXEDIR)/dir: $(OBJDIR)/dir.o $(LIBDIR)/libaros.a
	$(CC) $(ILDFLAGS) $^ -o $@ -L$(LIBDIR) -laros

$(EXEDIR)/librarytest: $(OBJDIR)/librarytest.o
	$(CC) $(ILDFLAGS) $^ -o $@

$(OBJDIR)/librarytest.o: librarytest.c $(TOP)/libs/dummylib_gcc.h
	$(CC) $(CFLAGS) -I $(TOP)/libs -c $< -o $@

$(EXEDIR)/%: %.o
	$(CC) $(ILDFLAGS) $< -o $@

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

