TOP=..

include $(TOP)/make.cfg

all: setup $(LIBDIR)/libdos.a

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; fi

clean:
	$(RM) $(OSGENDIR) *.err $(LIBDIR)/libdos.a

$(OSGENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

$(LIBDIR)/libdos.a: $(OSGENDIR)/dos_init.o \
	    $(OSGENDIR)/open.o $(OSGENDIR)/close.o $(OSGENDIR)/read.o $(OSGENDIR)/write.o \
	    $(OSGENDIR)/seek.o $(OSGENDIR)/adddosentry.o $(OSGENDIR)/finddosentry.o \
	    $(OSGENDIR)/addprocess.o $(OSGENDIR)/flush.o $(OSGENDIR)/output.o \
	    $(OSGENDIR)/allocdosobject.o $(OSGENDIR)/freedosentry.o \
	    $(OSGENDIR)/attemptlockdoslist.o $(OSGENDIR)/freedosobject.o \
	    $(OSGENDIR)/remdosentry.o $(OSGENDIR)/ioerr.o $(OSGENDIR)/createnewproc.o \
	    $(OSGENDIR)/isinteractive.o $(OSGENDIR)/setioerr.o $(OSGENDIR)/delay.o \
	    $(OSGENDIR)/loadseg_elf.o $(OSGENDIR)/unlockdoslist.o \
	    $(OSGENDIR)/lockdoslist.o $(OSGENDIR)/makedosentry.o $(OSGENDIR)/loadseg.o \
	    $(OSGENDIR)/input.o $(OSGENDIR)/selectoutput.o $(OSGENDIR)/selectinput.o \
	    $(OSGENDIR)/unloadseg.o $(OSGENDIR)/runcommand.o \
	    $(OSGENDIR)/readitem.o $(OSGENDIR)/fgetc.o $(OSGENDIR)/ungetc.o \
	    $(OSGENDIR)/readargs.o $(OSGENDIR)/strtolong.o $(OSGENDIR)/errorlist.o \
	    $(OSGENDIR)/freeargs.o $(OSGENDIR)/printfault.o $(OSGENDIR)/fputc.o \
	    $(OSGENDIR)/fputs.o $(OSGENDIR)/findarg.o $(OSGENDIR)/currentdir.o \
	    $(OSGENDIR)/namefromlock.o $(OSGENDIR)/assignlock.o $(OSGENDIR)/examine.o \
	    $(OSGENDIR)/putstr.o $(OSGENDIR)/vfprintf.o $(OSGENDIR)/vprintf.o \
	    $(OSGENDIR)/exall.o $(OSGENDIR)/datetostr.o $(OSGENDIR)/lddemon.o \
	    $(OSGENDIR)/getargstr.o $(OSGENDIR)/maxcli.o $(OSGENDIR)/cli.o \
	    $(OSGENDIR)/duplock.o $(OSGENDIR)/openfromlock.o \
	    $(OSGENDIR)/dos_functable.o
	$(AR) $@ $^
