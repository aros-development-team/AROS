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

#$(EXEDIR)/dir: $(OBJDIR)/dir.o $(OBJDIR)/strlen.o $(OBJDIR)/qsort.o $(OBJDIR)/strcpy.o
#	 $(CC) -Xlinker -i $(CFLAGS) $^ -o $@ -nostartfiles -nostdlib

$(EXEDIR)/dir: dir.c strlen.c qsort.c strcpy.c
	$(CC) -DNO_LINK $(CFLAGS) -c $< -o $@

$(EXEDIR)/librarytest: librarytest.c $(TOP)/libs/dummylib_gcc.h
	$(CC) $(CFLAGS) -I $(TOP)/libs -c $< -o $@

$(EXEDIR)/%: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

