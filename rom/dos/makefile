TOP=..

include $(TOP)/make.cfg

ODIR=$(OBJDIR)/dos

all: setup $(LIBDIR)/libdos.a

setup :
	@echo "Setting up"
	@if [ ! -d $(ODIR) ]; then $(MKDIR) $(ODIR) ; fi
	@$(TOUCH) setup

clean:
	$(RM) $(ODIR) *.err $(LIBDIR)/libdos.a setup

$(ODIR)/%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

$(ODIR)/%.o: %.s
	$(CC) $(CFLAGS) $^ -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

$(LIBDIR)/libdos.a: $(ODIR)/dos_init.o \
	    $(ODIR)/open.o $(ODIR)/close.o $(ODIR)/read.o $(ODIR)/write.o \
	    $(ODIR)/seek.o $(ODIR)/adddosentry.o $(ODIR)/finddosentry.o \
	    $(ODIR)/addprocess.o $(ODIR)/flush.o $(ODIR)/output.o \
	    $(ODIR)/allocdosobject.o $(ODIR)/freedosentry.o \
	    $(ODIR)/attemptlockdoslist.o $(ODIR)/freedosobject.o \
	    $(ODIR)/remdosentry.o $(ODIR)/ioerr.o $(ODIR)/createnewproc.o \
	    $(ODIR)/isinteractive.o $(ODIR)/setioerr.o $(ODIR)/delay.o \
	    $(ODIR)/loadseg_elf.o $(ODIR)/unlockdoslist.o \
	    $(ODIR)/lockdoslist.o $(ODIR)/makedosentry.o $(ODIR)/loadseg.o \
	    $(ODIR)/input.o $(ODIR)/selectoutput.o $(ODIR)/selectinput.o \
	    $(ODIR)/unloadseg.o $(ODIR)/runcommand.o $(ODIR)/runprocess.o \
	    $(ODIR)/readitem.o $(ODIR)/fgetc.o $(ODIR)/ungetc.o \
	    $(ODIR)/readargs.o $(ODIR)/strtolong.o $(ODIR)/errorlist.o \
	    $(ODIR)/freeargs.o $(ODIR)/printfault.o $(ODIR)/fputc.o \
	    $(ODIR)/fputs.o $(ODIR)/findarg.o $(ODIR)/currentdir.o \
	    $(ODIR)/namefromlock.o $(ODIR)/assignlock.o $(ODIR)/examine.o \
	    $(ODIR)/putstr.o $(ODIR)/vfprintf.o $(ODIR)/vprintf.o \
	    $(ODIR)/exall.o $(ODIR)/datetostr.o $(ODIR)/lddemon.o \
	    $(ODIR)/getargstr.o $(ODIR)/maxcli.o $(ODIR)/cli.o \
	    $(ODIR)/duplock.o $(ODIR)/openfromlock.o \
	    $(ODIR)/dos_functable.o
	$(AR) $@ $^
