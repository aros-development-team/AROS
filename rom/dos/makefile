# $Id$
TOP=..

include $(TOP)/make.cfg

FILES = dos_init dos_functable lddemon errorlist loadseg_elf doname
FUNCTIONS = \
	adddosentry addprocess allocdosobject assignlock attemptlockdoslist \
	cli close createnewproc currentdir datetostr delay duplock exall    \
	examine fgetc findarg finddosentry flush fputc fputs freeargs	    \
	freedosentry freedosobject getargstr input ioerr isinteractive	    \
	loadseg lockdoslist makedosentry maxcli namefromlock open	    \
	openfromlock output printfault putstr read readargs readitem	    \
	remdosentry runcommand seek selectinput selectoutput setioerr	    \
	setowner strtolong ungetc unloadseg unlockdoslist vfprintf	    \
	vprintf write filepart addbuffers changemode createdir deletefile   \
	exallend isfilesystem lock matchpattern matchpatternnocase	    \
	parsepattern parsepatternnocase setcomment setfiledate setfilesize  \
	setprotection

all: setup \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o) \
	$(foreach f,$(FUNCTIONS),$(OSGENDIR)/$(f).o)

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi

clean:
	$(RM) $(OSGENDIR) *.err $(LIBDIR)/libdos.a

$(OSGENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err ; else true ; fi

dos_functable.c : $(foreach f,$(FUNCTIONS),$(f).c) \
	    $(TOP)/scripts/makefunctable.awk
	gawk -f $(TOP)/scripts/makefunctable.awk \
	    --assign lib=Dos \
	    $^

$(OSGENDIR)/%.d: %.c
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -f$@ -p$(OSGENDIR)/ -- $(CFLAGS) -- $^

include $(foreach f,$(FILES),$(OSGENDIR)/$(f).d)
include $(foreach f,$(FUNCTIONS),$(OSGENDIR)/$(f).d)

