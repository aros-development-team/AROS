TOP=..

include $(TOP)/make.cfg

ODIR=$(OBJDIR)/exec

all: setup $(LIBDIR)/libexec.a

setup :
	@echo "Setting up"
	@if [ ! -d $(ODIR) ]; then $(MKDIR) $(ODIR) ; fi
	@$(TOUCH) setup

clean:
	$(RM) $(ODIR) *.err $(LIBDIR)/libexec.a setup

$(ODIR)/%.o: %.s
	$(CC) $(CFLAGS) $^ -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

$(ODIR)/%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

$(LIBDIR)/libexec.a: $(ODIR)/execfunctions.o $(ODIR)/exception.o \
	    $(ODIR)/allocmem.o $(ODIR)/freemem.o $(ODIR)/addhead.o \
	    $(ODIR)/addtask.o $(ODIR)/remtask.o $(ODIR)/wait.o \
	    $(ODIR)/signal.o $(ODIR)/allocsignal.o $(ODIR)/freesignal.o \
	    $(ODIR)/alert.o $(ODIR)/freeentry.o $(ODIR)/remove.o \
	    $(ODIR)/remhead.o $(ODIR)/enqueue.o $(ODIR)/addmemhandler.o \
	    $(ODIR)/setexcept.o $(ODIR)/setsignal.o $(ODIR)/settaskpri.o \
	    $(ODIR)/findtask.o $(ODIR)/findname.o $(ODIR)/getmsg.o \
	    $(ODIR)/putmsg.o $(ODIR)/replymsg.o $(ODIR)/addtail.o \
	    $(ODIR)/waitport.o $(ODIR)/allocate.o $(ODIR)/addport.o \
	    $(ODIR)/remport.o $(ODIR)/findport.o $(ODIR)/addmemlist.o \
	    $(ODIR)/createmsgport.o $(ODIR)/deletemsgport.o \
	    $(ODIR)/createiorequest.o $(ODIR)/deallocate.o \
	    $(ODIR)/availmem.o $(ODIR)/deleteiorequest.o $(ODIR)/allocvec.o \
	    $(ODIR)/freevec.o $(ODIR)/allocabs.o $(ODIR)/allocentry.o \
	    $(ODIR)/createpool.o $(ODIR)/deletepool.o $(ODIR)/allocpooled.o \
	    $(ODIR)/remmemhandler.o $(ODIR)/initstruct.o \
	    $(ODIR)/initsemaphore.o $(ODIR)/attemptsemaphore.o \
	    $(ODIR)/obtainsemaphore.o $(ODIR)/obtainsemaphoreshared.o \
	    $(ODIR)/attemptsemaphoreshared.o $(ODIR)/releasesemaphore.o \
	    $(ODIR)/procure.o $(ODIR)/vacate.o $(ODIR)/semaphoreglue.o \
	    $(ODIR)/addlibrary.o $(ODIR)/remlibrary.o \
	    $(ODIR)/makefunctions.o $(ODIR)/makelibrary.o \
	    $(ODIR)/sumlibrary.o $(ODIR)/openlibrary.o \
	    $(ODIR)/closelibrary.o $(ODIR)/setfunction.o \
	    $(ODIR)/oldopenlibrary.o $(ODIR)/copymemquick.o \
	    $(ODIR)/copymem.o $(ODIR)/setsr.o $(ODIR)/getcc.o \
	    $(ODIR)/cachecleare.o $(ODIR)/cacheclearu.o \
	    $(ODIR)/cachecontrol.o $(ODIR)/cachepostdma.o \
	    $(ODIR)/cachepredma.o $(ODIR)/freepooled.o \
	    $(ODIR)/remsemaphore.o $(ODIR)/addsemaphore.o \
	    $(ODIR)/releasesemaphorelist.o $(ODIR)/obtainsemaphorelist.o \
	    $(ODIR)/typeofmem.o $(ODIR)/rawdofmt.o $(ODIR)/insert.o \
	    $(ODIR)/remtail.o $(ODIR)/findsemaphore.o $(ODIR)/superstate.o \
	    $(ODIR)/userstate.o $(ODIR)/stackswap.o $(ODIR)/addresource.o \
	    $(ODIR)/remresource.o $(ODIR)/openresource.o \
	    $(ODIR)/adddevice.o $(ODIR)/remdevice.o $(ODIR)/opendevice.o \
	    $(ODIR)/closedevice.o $(ODIR)/doio.o $(ODIR)/sendio.o \
	    $(ODIR)/checkio.o $(ODIR)/abortio.o $(ODIR)/waitio.o \
	    $(ODIR)/initresident.o
	$(AR) $@ $^

