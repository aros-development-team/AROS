TOP=..

include $(TOP)/make.cfg

all: setup $(OSGENDIR)/execfunctions.o \
	    $(OSGENDIR)/allocmem.o $(OSGENDIR)/freemem.o $(OSGENDIR)/addhead.o \
	    $(OSGENDIR)/addtask.o $(OSGENDIR)/remtask.o $(OSGENDIR)/wait.o \
	    $(OSGENDIR)/signal.o $(OSGENDIR)/allocsignal.o $(OSGENDIR)/freesignal.o \
	    $(OSGENDIR)/alert.o $(OSGENDIR)/freeentry.o $(OSGENDIR)/remove.o \
	    $(OSGENDIR)/remhead.o $(OSGENDIR)/enqueue.o $(OSGENDIR)/addmemhandler.o \
	    $(OSGENDIR)/setexcept.o $(OSGENDIR)/setsignal.o $(OSGENDIR)/settaskpri.o \
	    $(OSGENDIR)/findtask.o $(OSGENDIR)/findname.o $(OSGENDIR)/getmsg.o \
	    $(OSGENDIR)/putmsg.o $(OSGENDIR)/replymsg.o $(OSGENDIR)/addtail.o \
	    $(OSGENDIR)/waitport.o $(OSGENDIR)/allocate.o $(OSGENDIR)/addport.o \
	    $(OSGENDIR)/remport.o $(OSGENDIR)/findport.o $(OSGENDIR)/addmemlist.o \
	    $(OSGENDIR)/createmsgport.o $(OSGENDIR)/deletemsgport.o \
	    $(OSGENDIR)/createiorequest.o $(OSGENDIR)/deallocate.o \
	    $(OSGENDIR)/availmem.o $(OSGENDIR)/deleteiorequest.o $(OSGENDIR)/allocvec.o \
	    $(OSGENDIR)/freevec.o $(OSGENDIR)/allocabs.o $(OSGENDIR)/allocentry.o \
	    $(OSGENDIR)/createpool.o $(OSGENDIR)/deletepool.o $(OSGENDIR)/allocpooled.o \
	    $(OSGENDIR)/remmemhandler.o $(OSGENDIR)/initstruct.o \
	    $(OSGENDIR)/initsemaphore.o $(OSGENDIR)/attemptsemaphore.o \
	    $(OSGENDIR)/obtainsemaphore.o $(OSGENDIR)/obtainsemaphoreshared.o \
	    $(OSGENDIR)/attemptsemaphoreshared.o $(OSGENDIR)/releasesemaphore.o \
	    $(OSGENDIR)/procure.o $(OSGENDIR)/vacate.o \
	    $(OSGENDIR)/addlibrary.o $(OSGENDIR)/remlibrary.o \
	    $(OSGENDIR)/makefunctions.o $(OSGENDIR)/makelibrary.o \
	    $(OSGENDIR)/sumlibrary.o $(OSGENDIR)/openlibrary.o \
	    $(OSGENDIR)/closelibrary.o $(OSGENDIR)/setfunction.o \
	    $(OSGENDIR)/oldopenlibrary.o $(OSGENDIR)/copymemquick.o \
	    $(OSGENDIR)/copymem.o $(OSGENDIR)/freepooled.o \
	    $(OSGENDIR)/remsemaphore.o $(OSGENDIR)/addsemaphore.o \
	    $(OSGENDIR)/releasesemaphorelist.o $(OSGENDIR)/obtainsemaphorelist.o \
	    $(OSGENDIR)/typeofmem.o $(OSGENDIR)/rawdofmt.o $(OSGENDIR)/insert.o \
	    $(OSGENDIR)/remtail.o $(OSGENDIR)/findsemaphore.o \
	    $(OSGENDIR)/addresource.o \
	    $(OSGENDIR)/remresource.o $(OSGENDIR)/openresource.o \
	    $(OSGENDIR)/adddevice.o $(OSGENDIR)/remdevice.o $(OSGENDIR)/opendevice.o \
	    $(OSGENDIR)/closedevice.o $(OSGENDIR)/doio.o $(OSGENDIR)/sendio.o \
	    $(OSGENDIR)/checkio.o $(OSGENDIR)/abortio.o $(OSGENDIR)/waitio.o \
	    $(OSGENDIR)/initresident.o

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; fi

clean:
	$(RM) $(OSGENDIR) *.err $(LIBDIR)/libexec.a

$(OSGENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

