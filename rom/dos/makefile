# $Id$
TOP=../..

# Use OS's own includes over the ones from AROS
SPECIAL_CFLAGS=-I/usr/include

include $(TOP)/config/make.cfg

EXEDIR=$(BINDIR)/test
TESTOBJDIR = $(GENDIR)/rom/dos
LIBS = -L$(LIBDIR) -lamiga -laros -larosc
DEPLIBS = $(LIBDIR)/libamiga.a $(LIBDIR)/libaros.a $(LIBDIR)/startup.o \
	$(LIBDIR)/libarosc.a

OBJDIR=$(OSGENDIR)/dos
LIB=$(LIBDIR)/libdos.so

FILES = dos_init dos_functable lddemon errorlist loadseg_elf doname \
	loadseg_aout

FUNCTIONS = \
	abortpkt addbuffers adddosentry addpart addprocess addsegment	     \
	allocdosobject assignadd assignlate	       \
	assignlock assignpath attemptlockdoslist changemode checksignal cli  \
	cliinitnewcli cliinitrun close comparedates createdir createnewproc  \
	createproc currentdir datestamp datetostr	\
	delay deletefile deletevar deviceproc dopkt	\
	duplock endnotify errorreport	    \
	exall exallend examine execute exit exnext fault fgetc	   \
	fgets filepart findarg findcliproc finddosentry findsegment findvar  \
	flush format fputc fputs fread freeargs freedeviceproc	     \
	freedosentry freedosobject fwrite getargstr getconsoletask   \
	getcurrentdirname getdeviceproc getfilesystask getprogramdir	     \
	getprogramname getprompt getvar info inhibit input internalloadseg   \
	internalunloadseg ioerr isfilesystem isinteractive loadseg   \
	lock lockdoslist lockrecord lockrecords makedosentry	 \
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
	vfwritef vprintf waitforchar waitpkt write writechars \
	dosgetstring

OBJS = $(foreach f,$(FILES) $(FUNCTIONS),$(OBJDIR)/$(f).o)

all: setup $(LIB)

setup :
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi
	@if [ ! -d $(LIBDIR) ]; then $(MKDIR) $(LIBDIR) ; else true ; fi
	@echo "CFLAGS=$(CFLAGS)"

clean:
	$(RM) $(OBJDIR) *.err $(LIB)

TESTS = datetostr strtodate splitname

test: setup-test $(foreach f,$(TESTS),$(EXEDIR)/$(f))

$(LIB) : $(OBJS)
	@echo "Recreating $@..."
	@$(SHARED_AR) $@ $(OBJS)

$(EXEDIR)/%: %.c $(DEPLIBS)
	$(CC) $(ILDFLAGS) -DTEST $(COMMON_CFLAGS) $(LIBDIR)/startup.o \
	    $< -o $@ $(LIBS)

setup-test :
	@if [ ! -d $(TESTOBJDIR) ]; then $(MKDIR) $(TESTOBJDIR) ; else true ; fi
	@if [ ! -d $(EXEDIR) ]; then $(MKDIR) $(EXEDIR) ; else true ; fi

$(OBJDIR)/%.o: %.c
	@echo "Compiling $(CURDIR)/$<..."
	@$(CC) $(SHARED_CFLAGS) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

dos_functable.c : $(foreach f,$(FUNCTIONS),$(f).c) \
	    $(TOP)/scripts/makefunctable.awk
	gawk -f $(TOP)/scripts/makefunctable.awk \
	    --assign lib=Dos \
	    $^

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	@echo "Makedepend $CURDIR/$<..."
	@$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

ifneq ($(TARGET),clean)
include $(foreach f,$(FILES) $(FUNCTIONS),$(OBJDIR)/$(f).d)
endif
