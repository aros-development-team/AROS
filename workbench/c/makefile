TOP=..

include $(TOP)/make.cfg

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
	$(EXEDIR)/wait

all: $(EXES)

clean:
	$(RM) $(EXES)

$(BINDIR)/dir: dir.c strlen.c qsort.c strcpy.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BINDIR)/librarytest: librarytest.c $(TOP)/libs/dummylib_gcc.h
	$(CC) $(CFLAGS) -I $(TOP)/libs -c $< -o $@

$(BINDIR)/%: %.c
	$(CC) $(CFLAGS) -c $< -o $@

