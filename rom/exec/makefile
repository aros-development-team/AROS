TOP=..

include $(TOP)/make.cfg

FILES = init execfunctions \
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
	initresident cause

all: setup $(foreach f,$(FILES),$(OSGENDIR)/$(f).o)

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; fi

clean:
	$(RM) $(OSGENDIR) *.err $(LIBDIR)/libexec.a

$(OSGENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

$(OSGENDIR)/%.d: %.c
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; fi
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -f$@ -p$(OSGENDIR)/ -- $(CFLAGS) -- $^

include $(foreach f,$(FILES),$(OSGENDIR)/$(f).d)
