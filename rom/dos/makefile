TOP=..

include $(TOP)/make.cfg

FILES = adddosentry addprocess allocdosobject assignlock attemptlockdoslist \
	cli close createnewproc currentdir datetostr delay duplock exall    \
	examine fgetc findarg finddosentry flush fputc fputs freeargs	    \
	freedosentry freedosobject getargstr input ioerr isinteractive	    \
	loadseg lockdoslist makedosentry maxcli namefromlock open	    \
	openfromlock output printfault putstr read readargs readitem	    \
	remdosentry runcommand seek selectinput selectoutput setioerr	    \
	setowner strtolong ungetc unloadseg unlockdoslist vfprintf	    \
	vprintf write

all: setup $(OSGENDIR)/dos_init.o $(OSGENDIR)/dos_functable.o \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o) \
	$(OSGENDIR)/lddemon.o \
	$(OSGENDIR)/errorlist.o \
	$(OSGENDIR)/loadseg_elf.o \

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; fi

clean:
	$(RM) $(OSGENDIR) *.err $(LIBDIR)/libdos.a

$(OSGENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

dos_functable.c : $(foreach f,$(FILES),$(f).c) \
	    $(TOP)/scripts/makefunctable.awk
	gawk -f $(TOP)/scripts/makefunctable.awk \
	    --assign lib=Dos \
	    $(foreach f,$(FILES),$(f).c)
