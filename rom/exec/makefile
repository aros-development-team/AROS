# $Id$
TOP=..

# Use OS's own includes over the local ones
SPECIAL_CFLAGS=-I/usr/include

include $(TOP)/config/make.cfg

ifeq ($(FLAVOUR),native)
LIBS = -L$(LIBDIR) -larosc

FILES = execstrap_res execstrap_init execstrap_end

FUNCTIONS = \
	addintserver remintserver \
	typeofmem \
	addmemlist addmemhandler remmemhandler \
	insert addhead addtail remove remhead remtail enqueue findname \
	setsignal \
	addport remport waitport findport \
	putmsg getmsg \
	doio sendio checkio waitio abortio createiorequest deleteiorequest \
	addresource remresource openresource \
	addlibrary remlibrary oldopenlibrary closelibrary setfunction \
	sumlibrary openlibrary taggedopenlibrary \
	adddevice \
	findtask \
	attemptsemaphore findsemaphore addsemaphore remsemaphore \
	obtainsemaphoreshared attemptsemaphoreshared

# Functions that are written in asm and need to be linked into exec.strap.
# These are in config/$(KERNEL).
ASMFUNCTIONS = forbid permit disable getcc cacheclearu

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

$(SLIBDIR)/exec.strap:
	$(CC) $(ILDFLAGS) \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o) \
	$(foreach f,$(FUNCTIONS),$(OSGENDIR)/$(f).o) \
	$(foreach f,$(ASMFUNCTIONS),$(OSGENDIR)/$(f).o) \
	$(LIBS) \
	-o $@ 2>&1|tee exec.strap.err
	@if test ! -s exec.strap.err; then rm exec.strap.err ; else true ; fi
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
	$(CC) -S $(CFLAGS) $< -c -o $@ 2>&1|tee $*.errs
	@if test ! -s $*.errs; then rm $*.errs ; else true ; fi
endif

$(OSGENDIR)/%.d: %.c
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -f$@ -p$(OSGENDIR)/ -- $(CFLAGS) -- $^

ifeq ($(FLAVOUR),native)
# Dependencies for exec.strap:
$(SLIBDIR)/exec.strap: $(foreach f,$(FILES),$(OSGENDIR)/$(f).o) \
		       $(foreach f,$(FUNCTIONS),$(OSGENDIR)/$(f).o) \
		       $(foreach f,$(ASMFUNCTIONS),$(OSGENDIR)/$(f).o)
endif

# Other dependencies:
include $(foreach f,$(FILES),$(OSGENDIR)/$(f).d)
include $(foreach f,$(FUNCTIONS),$(OSGENDIR)/$(f).d)

