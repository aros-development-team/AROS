# $Id$
TOP=../..
COMPILE_KERNEL = yes # Must have a value, no matter which one

# Use OS's own includes over the ones from AROS
SPECIAL_CFLAGS=-I/usr/include

include $(TOP)/config/make.cfg

ifeq ($(FLAVOUR),native)
LIBS = -L$(LIBDIR) -larosc

FILES = execstrap_res execstrap_init execstrap_end

FUNCTIONS = \
	initstruct makelibrary makefunctions findresident initresident \
	setintvector addintserver remintserver \
	allocmem freemem availmem allocentry freeentry typeofmem \
	addmemlist addmemhandler remmemhandler \
	insert addhead addtail remove remhead remtail enqueue findname \
	setsignal allocsignal freesignal \
	addport remport waitport findport createmsgport deletemsgport \
	putmsg getmsg replymsg \
	doio sendio checkio waitio abortio createiorequest deleteiorequest \
	addresource remresource openresource \
	addlibrary remlibrary oldopenlibrary closelibrary \
	setfunction sumlibrary openlibrary taggedopenlibrary \
	adddevice remdevice \
	findtask \
	rawdofmt \
	allocvec freevec \
	initsemaphore attemptsemaphore findsemaphore addsemaphore remsemaphore \
	obtainsemaphoreshared attemptsemaphoreshared

# Functions that are written in asm and need to be linked into exec.strap.
# These are in config/$(KERNEL).
ASMFUNCTIONS = forbid disable getcc cacheclearu cachepredma cachepostdma

else
LIBS =

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
	rawputchar addintserver \
	remintserver setintvector \
	alloctrap childfree childorphan childstatus childwait coldreboot \
	debug findresident freetrap initcode sumkickdata obtainquickvector
endif

ifeq ($(FLAVOUR),native)
all: setup \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o) \
	$(foreach f,$(FUNCTIONS),$(OSGENDIR)/$(f).o) \
	$(foreach f,$(FUNCTIONS),$(f).s) \
	$(SLIBDIR)/exec.strap

$(SLIBDIR)/exec.strap: $(foreach f,$(FILES),$(OSGENDIR)/$(f).o) \
		       $(foreach f,$(FUNCTIONS),$(OSGENDIR)/$(f).o) \
		       $(foreach f,$(ASMFUNCTIONS),$(OSGENDIR)/$(f).o)
	$(CC) $(ILDFLAGS) \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o) \
	$(foreach f,$(FUNCTIONS),$(OSGENDIR)/$(f).o) \
	$(foreach f,$(ASMFUNCTIONS),$(OSGENDIR)/$(f).o) \
	$(LIBS) -o $@ 2>&1|tee execstrap.err
	@if test ! -s execstrap.err; then rm execstrap.err ; else true ; fi
	@strip $@
else
all: setup \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o) \
	$(foreach f,$(FUNCTIONS),$(OSGENDIR)/$(f).o)
endif

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi

clean:
	$(RM) $(OSGENDIR) *.err $(LIBDIR)/libexec.a

$(OSGENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

ifeq ($(FLAVOUR),native)
# To study the assembler output for evaluation (can be helpful in tracking
# down bugs):
%.s: %.c
	$(CC) -S $(CFLAGS) $< -c -o $@
endif

$(OSGENDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

ifneq ($(TARGET),clean)
include $(foreach f,$(FILES) $(FUNCTIONS),$(OSGENDIR)/$(f).d)
endif
