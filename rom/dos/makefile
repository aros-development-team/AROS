# $Id$
TOP=../..

# Use OS's own includes over the ones from AROS
SPECIAL_CFLAGS=-I/usr/include

include $(TOP)/config/make.cfg

FILES = dos_init dos_functable lddemon errorlist loadseg_elf doname \
	loadseg_aout

FUNCTIONS = \
	abortpkt addbuffers adddosentry addpart addprocess addsegment	     \
	allocdosobject assignadd assignlate	       \
	assignlock assignpath attemptlockdoslist changemode checksignal cli  \
	cliinitnewcli cliinitrun close comparedates createdir createnewproc  \
	createproc currentdir datestamp datetostr	\
	delay deletefile deletevar deviceproc doname dopkt     \
	duplock endnotify errorlist errorreport       \
	exall exallend examine execute exit exnext fault fgetc	   \
	fgets filepart findarg findcliproc finddosentry findsegment findvar  \
	flush format fputc fputs fread freeargs freedeviceproc	     \
	freedosentry freedosobject fwrite getargstr getconsoletask   \
	getcurrentdirname getdeviceproc getfilesystask getprogramdir	     \
	getprogramname getprompt getvar info inhibit input internalloadseg   \
	internalunloadseg ioerr isfilesystem isinteractive lddemon loadseg   \
	loadseg_elf lock lockdoslist lockrecord lockrecords makedosentry     \
	makelink matchend matchfirst matchnext matchpattern		     \
	matchpatternnocase maxcli namefromlock newloadseg	  \
	nextdosentry open openfromlock output parentdir    \
	parentoffh parsepattern parsepatternnocase pathpart printfault	     \
	putstr read readargs readitem readlink relabel remassignlist	     \
	remdosentry remsegment rename replypkt runcommand samedevice	     \
	samelock seek selectinput selectoutput sendpkt setargstr setcomment  \
	setconsoletask setcurrentdirname setfiledate setfilesize	     \
	setfilesystask setioerr setmode setowner setprogramdir		     \
	setprogramname setprompt setprotection setvar setvbuf splitname      \
	startnotify strtodate strtolong systemtaglist ungetc	      \
	unloadseg unlockdoslist unlockrecord unlockrecords vfprintf   \
	vfwritef vprintf waitforchar waitpkt write writechars

all: setup \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o) \
	$(foreach f,$(FUNCTIONS),$(OSGENDIR)/$(f).o)

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi

clean:
	$(RM) $(OSGENDIR) *.err $(LIBDIR)/libdos.a

$(OSGENDIR)/%.o: %.c
	$(CC) $(SHARED_CFLAGS) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

dos_functable.c : $(foreach f,$(FUNCTIONS),$(f).c) \
	    $(TOP)/scripts/makefunctable.awk
	gawk -f $(TOP)/scripts/makefunctable.awk \
	    --assign lib=Dos \
	    $^

$(OSGENDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

ifneq ($(TARGET),clean)
include $(foreach f,$(FILES) $(FUNCTIONS),$(OSGENDIR)/$(f).d)
endif
