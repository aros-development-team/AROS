TOP=..
RM=rm -rf
CFLAGS=$(COMMON_CFLAGS) -I $(TOP)/include -I $(TOP)/amiga/include

all: libdos.a

clean:
	$(RM) *.o *.a

%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

%.o: %.s
	$(CC) $(CFLAGS) $^ -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

libdos.a: dos_init.o \
	  open.o close.o read.o write.o seek.o adddosentry.o finddosentry.o \
	  addprocess.o flush.o output.o allocdosobject.o freedosentry.o \
	  attemptlockdoslist.o freedosobject.o remdosentry.o ioerr.o \
	  createnewproc.o isinteractive.o setioerr.o delay.o loadseg_elf.o \
	  unlockdoslist.o lockdoslist.o makedosentry.o loadseg.o input.o \
	  selectoutput.o selectinput.o unloadseg.o runcommand.o \
	  runprocess.o readitem.o fgetc.o ungetc.o readargs.o \
	  strtolong.o errorlist.o freeargs.o printfault.o fputc.o \
	  fputs.o findarg.o currentdir.o namefromlock.o assignlock.o \
	  examine.o putstr.o vfprintf.o vprintf.o exall.o datetostr.o \
	  lddemon.o getargstr.o maxcli.o cli.o duplock.o openfromlock.o \
	  dos_functable.o
	ar -r $@ $^
