# $Id$
TOP=../..

# Use OS's own includes over the ones from AROS
SPECIAL_CFLAGS=-I/usr/include

include $(TOP)/config/make.cfg

ifeq ($(FLAVOUR),native)
OBJDIR=$(OSGENDIR)

FILES = execstrap_init

FUNCTIONS = \
	initcode initstruct makelibrary makefunctions findresident	    \
	initresident setintvector addintserver remintserver allocmem	    \
	freemem availmem allocentry freeentry typeofmem addmemlist	    \
	addmemhandler remmemhandler insert addhead addtail remove remhead   \
	remtail enqueue findname setsignal allocsignal freesignal addport   \
	remport waitport findport createmsgport deletemsgport putmsg getmsg \
	replymsg doio sendio checkio waitio abortio createiorequest	    \
	deleteiorequest addresource remresource openresource addlibrary     \
	remlibrary oldopenlibrary closelibrary setfunction sumlibrary	    \
	openlibrary taggedopenlibrary adddevice remdevice opendevice	    \
	closedevice findtask rawdofmt allocvec freevec initsemaphore	    \
	attemptsemaphore findsemaphore addsemaphore remsemaphore	    \
	obtainsemaphoreshared attemptsemaphoreshared createpool deletepool  \
	allocpooled freepooled

# Functions that are written in asm and need to be linked into exec.strap.
# These are in config/$(KERNEL).
ASMFUNCTIONS = forbid disable getcc cacheclearu cachepredma cachepostdma

else
OBJDIR=$(OSGENDIR)/exec
LIB=$(LIBDIR)/libexec.so

FILES = init execfunctions

FUNCTIONS = \
	allocmem freemem addhead \
	addtask remtask wait \
	signal allocsignal freesignal \
	alert freeentry remove \
	remhead enqueue addmemhandler \
	setexcept setsignal settaskpri \
	findtask findname getmsg \
	putmsg replymsg addtail \
	waitport allocate addport \
	remport findport addmemlist \
	createmsgport deletemsgport \
	createiorequest deallocate \
	availmem deleteiorequest allocvec \
	freevec allocabs allocentry \
	createpool deletepool allocpooled \
	remmemhandler initstruct \
	initsemaphore attemptsemaphore \
	obtainsemaphore obtainsemaphoreshared \
	attemptsemaphoreshared releasesemaphore \
	procure vacate \
	addlibrary remlibrary \
	makefunctions makelibrary \
	sumlibrary openlibrary \
	closelibrary setfunction \
	oldopenlibrary copymemquick \
	copymem freepooled \
	remsemaphore addsemaphore \
	releasesemaphorelist obtainsemaphorelist \
	typeofmem rawdofmt insert \
	remtail findsemaphore \
	addresource \
	remresource openresource \
	adddevice remdevice opendevice \
	closedevice doio sendio \
	checkio abortio waitio \
	initresident cause \
	addintserver \
	remintserver setintvector \
	alloctrap childfree childorphan childstatus childwait coldreboot \
	debug findresident freetrap initcode sumkickdata obtainquickvector
endif

ifeq ($(FLAVOUR),native)
LIBS = -L$(LIBDIR) -laros -larosc

all: setup $(OSMODDIR)/exec.strap

$(OSMODDIR)/exec.strap: $(foreach f,$(FILES),$(OBJDIR)/$(f).o) \
			$(foreach f,$(FUNCTIONS),$(OBJDIR)/$(f).o) \
			$(foreach f,$(ASMFUNCTIONS),$(OBJDIR)/$(f).o) \
			$(LIBDIR)/libaros.a $(LIBDIR)/libarosc.a
	$(CC) $(ILDFLAGS)  -Xlinker -M \
	$(foreach f,$(FILES),$(OBJDIR)/$(f).o) \
	$(foreach f,$(FUNCTIONS),$(OBJDIR)/$(f).o) \
	$(foreach f,$(ASMFUNCTIONS),$(OBJDIR)/$(f).o) \
	$(LIBS) -o $@ 2>&1|tee execstrap.err
	@if test ! -s execstrap.err; then rm execstrap.err ; else true ; fi
	@strip $@

# To study the assembler output for evaluation (can be helpful in tracking
# down bugs):
asm: all $(foreach f,$(FUNCTIONS),$(f).s)

%.s: %.c
	$(CC) -S $(CFLAGS) $< -c -o $@
else

OBJS = $(foreach f,$(FILES) $(FUNCTIONS),$(OBJDIR)/$(f).o)

all: setup $(LIB)
endif

setup :
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi
	@if [ ! -d $(OSMODDIR) ]; then $(MKDIR) $(OSMODDIR) ; else true ; fi

clean:
	$(RM) $(OBJDIR) *.err $(LIBDIR)/libexec.a

$(LIB): $(OBJS)
	@echo "Recreating $@..."
	@$(SHARED_AR) $@ $(OBJS)

$(OBJDIR)/%.o: %.c
	$(CC) $(SHARED_CFLAGS) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

ifneq ($(TARGET),clean)
include $(foreach f,$(FILES) $(FUNCTIONS),$(OBJDIR)/$(f).d)
endif
