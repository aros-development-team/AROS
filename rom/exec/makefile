TOP=..
RM=rm -rf
CFLAGS=$(COMMON_CFLAGS) -I $(TOP)/include -I $(TOP)/amiga/include

all: libexec.a

clean:
	$(RM) *.o libexec.a *.err

%.o: %.s
	$(CC) $(CFLAGS) $^ -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

libexec.a: execfunctions.o exception.o allocmem.o freemem.o addhead.o \
	   addtask.o remtask.o wait.o signal.o allocsignal.o freesignal.o \
	   alert.o freeentry.o remove.o remhead.o enqueue.o addmemhandler.o \
	   setexcept.o setsignal.o settaskpri.o findtask.o findname.o \
	   getmsg.o putmsg.o replymsg.o addtail.o waitport.o allocate.o \
	   addport.o remport.o findport.o addmemlist.o createmsgport.o \
	   deletemsgport.o createiorequest.o deallocate.o availmem.o \
	   deleteiorequest.o allocvec.o freevec.o allocabs.o allocentry.o \
	   createpool.o deletepool.o allocpooled.o remmemhandler.o \
	   initstruct.o initsemaphore.o attemptsemaphore.o obtainsemaphore.o \
	   obtainsemaphoreshared.o attemptsemaphoreshared.o releasesemaphore.o \
	   procure.o vacate.o semaphoreglue.o addlibrary.o remlibrary.o \
	   makefunctions.o makelibrary.o sumlibrary.o openlibrary.o \
	   closelibrary.o setfunction.o oldopenlibrary.o copymemquick.o copymem.o \
	   setsr.o getcc.o cachecleare.o cacheclearu.o cachecontrol.o \
	   cachepostdma.o cachepredma.o freepooled.o remsemaphore.o addsemaphore.o \
	   releasesemaphorelist.o obtainsemaphorelist.o typeofmem.o rawdofmt.o \
	   insert.o remtail.o findsemaphore.o superstate.o userstate.o \
	   stackswap.o addresource.o remresource.o openresource.o \
	   adddevice.o remdevice.o opendevice.o closedevice.o \
	   doio.o sendio.o checkio.o abortio.o waitio.o initresident.o
	ar -r $@ $^

