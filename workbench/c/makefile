TOP=..
RM=rm -rf
CFLAGS=$(COMMON_CFLAGS) -fomit-frame-pointer -Dmain=submain -I $(TOP)/include -I $(TOP)/amiga/include

EXES = $(BINDIR)/helloworld \
	$(BINDIR)/newshell \
	$(BINDIR)/type \
	$(BINDIR)/echo \
	$(BINDIR)/cd \
	$(BINDIR)/avail \
	$(BINDIR)/list \
	$(BINDIR)/librarytest \
	$(BINDIR)/run \
	$(BINDIR)/execute \
	$(BINDIR)/path \
	$(BINDIR)/shell \
	$(BINDIR)/dir \
	$(BINDIR)/wait

all: $(EXES)

clean:
	$(RM) $(EXES) core *.o

$(BINDIR)/dir: dir.c strlen.c qsort.c strcpy.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BINDIR)/%: %.c
	$(CC) $(CFLAGS) -c $< -o $@

