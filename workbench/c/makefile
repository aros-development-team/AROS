TOP=..

include $(TOP)/make.cfg

ODIR = $(OBJDIR)/exe

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
	@echo "Setting up"
	@if [ ! -d $(EXEDIR) ]; then $(MKDIR) $(EXEDIR) ; fi
	@if [ ! -d $(ODIR) ]; then $(MKDIR) $(ODIR) ; fi
	@touch setup

clean:
	$(RM) $(EXES) setup

$(EXEDIR)/forkbomb : forkbomb
	$(CP) $< $@

#$(EXEDIR)/dir: dir.o strlen.o qsort.o strcpy.o
#	 $(CC) -Xlinker -i $(CFLAGS) $^ -o $@ -nostartfiles -nostdlib

$(EXEDIR)/dir: dir.c strlen.c qsort.c strcpy.c
	$(CC) -DNO_LINK $(CFLAGS) -c $< -o $@

$(EXEDIR)/librarytest: librarytest.c $(TOP)/libs/dummylib_gcc.h
	$(CC) $(CFLAGS) -I $(TOP)/libs -c $< -o $@

$(EXEDIR)/%: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

