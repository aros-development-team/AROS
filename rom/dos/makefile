# $Id$
TOP=../..

# Use OS's own includes over the ones from AROS
ifeq ("$(SHARED_DOS)","yes")
SPECIAL_CFLAGS=$(SHARED_CFLAGS) -I/usr/include
else
SPECIAL_CFLAGS=-I/usr/include
endif

include $(TOP)/config/make.cfg

EXEDIR=$(BINDIR)/test
TESTOBJDIR = $(GENDIR)/rom/dos

STARTUP = $(LIBDIR)/startup.o
LIBS = -L$(LIBDIR)  -lamiga -larossupport -larosc 
DEPLIBS = $(LIBDIR)/libamiga.a $(LIBDIR)/libarossupport.a $(STARTUP) \
	$(LIBDIR)/libarosc.a 

OBJDIR=$(OSGENDIR)/dos
ifeq ("$(SHARED_DOS)","yes")
LIB=$(LIBDIR)/libdos.so
RMLIB=$(LIBDIR)/libdos.a
AR=$(SHARED_AR)
RANLIB=true
else
LIB=$(LIBDIR)/libdos.a
RMLIB=$(LIBDIR)/libdos.so
endif

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
DEPS = $(foreach f,$(FILES) $(FUNCTIONS),$(OBJDIR)/$(f).d)

all: setup $(LIB)

setup :
	@for dir in $(OBJDIR) $(LIBDIR) ; do \
	    if [ ! -d $$dir ]; then $(MKDIR) $$dir ; else true ; fi ; \
	done
	@echo "CFLAGS=$(CFLAGS)"

clean:
	$(RM) $(OBJDIR) *.err $(LIB)

TESTS = datetostr strtodate splitname readargs

test: setup-test $(foreach f,$(TESTS),$(EXEDIR)/$(f))

$(LIB) : $(OBJS)
	@$(ECHO) "Creating library $@..."
	@$(AR) $@ $(OBJS)
	@$(RANLIB) $@
	@$(RM) $(RMLIB)

TEST_LDFLAGS = $(ILDFLAGS) -DTEST $(COMMON_CFLAGS)

$(EXEDIR)/%: %.c $(DEPLIBS)
	@$(ECHO) "Linking $@..."
	@$(CC) $(TEST_LDFLAGS) $(STARTUP) $< -o $@ $(LIBS) 2>&1|tee $(@F)-x.err
	@if test ! -s $(@F)-x.err; then rm $(@F)-x.err ; else true ; fi

setup-test :
	@for dir in $(TESTOBJDIR) $(EXEDIR) ; do \
	    if [ ! -d $$dir ]; then $(MKDIR) $$dir ; else true ; fi ; \
	done

$(OBJDIR)/%.o: %.c
	@$(ECHO) "Compiling $(CURDIR)/$<..."
	@$(CC) $(CFLAGS) -c $< -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

dos_functable.c : $(foreach f,$(FUNCTIONS),$(f).c) \
	    $(TOP)/scripts/makefunctable.awk
	@$(ECHO) "Regenerating $(to)..."
	@$(AWK) -f $(TOP)/scripts/makefunctable.awk \
	    --assign lib="Dos" $^

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	@$(ECHO) "Makedepend $(CURDIR)/$<..."
	@$(MKDEPEND) -f- -p$(@D)/  -- $(CFLAGS) -- $< > $@

#	 @if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
#	 @echo "Makedepend $(CURDIR)/$<..."
#	 @$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

# Also check if the makefile itself must be remade
all : makefile

# Remake makefile
makefile : makefile.src $(TOP)/scripts/genmf.gawk \
	    $(TOP)/config/make.tmpl
	@$(ECHO) "Regenerating makefile..."
	@$(MV) $@ $@.bak
	@$(AWK) -f $(TOP)/scripts/genmf.gawk --assign TOP="$(TOP)" \
	    $< > $@

# Include these files but only if the current target is neither
# setup nor clean.
ifneq ("$(TARGET)","clean")
ifneq ("$(TARGET)","setup")
include $(DEPS)
endif
endif
