# (C) 1997-1998 AROS - The Amia Research OS

FUNCTIONS = abortio adddevice addhead addintserver addlibrary addmemhandler   \
    addmemlist addport addresource addsemaphore addtail addtask alert allocabs\
    allocate allocentry allocmem allocpooled allocsignal          allocvec    \
    attemptsemaphore attemptsemaphoreshared availmem                          \
                     checkio                                                  \
    closedevice closelibrary copymem copymemquick createiorequest createmsgport\
    createpool deallocate Debug deleteiorequest deletemsgport deletepool doio \
    enqueue            findname findport findresident findsemaphore findtask  \
    freeentry freemem freepooled freesignal          freevec getmsg initcode  \
    initresident initsemaphore initstruct insert makefunctions makelibrary    \
                      obtainsemaphore obtainsemaphorelist obtainsemaphoreshared\
    oldopenlibrary opendevice openlibrary openresource procure putmsg rawdofmt\
    rawioinit rawmaygetchar releasesemaphore releasesemaphorelist remdevice   \
    remhead remintserver remlibrary remmemhandler remove remport remresource  \
    remsemaphore remtail remtask replymsg reschedule sendio setexcept setfunction\
    setintvector setsignal settaskpri signal           sumkickdata sumlibrary \
          taggedopenlibrary             typeofmem vacate wait waitio waitport \
    exec_end \
    childfree childorphan childstatus childwait superstate userstate alloctrap\
    freetrap exception traphandler rawputchar cause obtainquickvector

INTERNAL = exec_init prepareexecbase execfunctions exec_util preparecontext

$(EXEC): $(foreach f,$(INTERNAL),$(f).c) functions.c
	@echo -n Compiling exec.library...
	@for i in $(INTERNAL); do $(CC) -c $$i.c; done
	@$(CC) -c functions.c 2>functions.err
	@$(LINK) $(foreach f,$(INTERNAL),$(f).o) functions.o -o $(EXEC)
	@echo Done!

functions.c:
	@$(CPAK) >/dev/null $(shell for i in $(FUNCTIONS); do \
	if [ -f "$$i.c" ]; then echo $$i; \
	else echo $(TOP)/rom/exec/$$i; fi; done)

%.s: %.c
	@gcc -S -O0 -mno-fp-ret-in-387 -I../../include -I../../../../compiler/include $<

clean:
	@rm -f *.o $(EXEC) functions.*