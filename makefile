KERNEL=i386-emul

TOP=.
CURDIR=.
COMMON_CFLAGS=-Wall -g
CFLAGS=$(COMMON_CFLAGS) -Dmain=submain -I $(TOP)/include -I $(TOP)/amiga/include
CC=gcc
RM=rm -rf

LIBS=$(KERNEL)/libkernel.a exec/libexec.a $(KERNEL)/libkernel.a dos/libdos.a \
filesys/emul_handler.o utility/libutility.a

%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@

all: subdirs arosshell apps

test: tasktest signaltest exceptiontest tasktest2 messagetest \
    semaphoretest initstructtest devicetest filetest

clean:
	$(RM) tasktest signaltest exceptiontest tasktest2 messagetest \
		semaphoretest initstructtest devicetest filetest \
		core *.o *.a
	@for dir in $(KERNEL) exec dos utility filesys libs ; do \
	    ( cd $$dir ; \
	    $(MAKE) clean ) ; \
	done

tasktest: tasktest.o $(LIBS)
	$(CC) $(CFLAGS) tasktest.o $(LIBS) -o $@

signaltest: signaltest.o $(LIBS)
	$(CC) $(CFLAGS) signaltest.o $(LIBS) -o $@

exceptiontest: exceptiontest.o $(LIBS)
	$(CC) $(CFLAGS) exceptiontest.o $(LIBS) -o $@

tasktest2: tasktest2.o $(LIBS)
	$(CC) $(CFLAGS) tasktest2.o $(LIBS) -o $@

messagetest: messagetest.o $(LIBS)
	$(CC) $(CFLAGS) messagetest.o $(LIBS) -o $@

initstructtest: initstructtest.o $(LIBS)
	$(CC) $(CFLAGS) initstructtest.o $(LIBS) -o $@

semaphoretest: semaphoretest.o $(LIBS)
	$(CC) $(CFLAGS) semaphoretest.o $(LIBS) -o $@

devicetest: devicetest.o dummydev.o $(LIBS)
	$(CC) $(CFLAGS) devicetest.o dummydev.o $(LIBS) -o $@

supertest: supertest.o $(LIBS)
	$(CC) $(CFLAGS) supertest.o $(LIBS) -o $@

filetest: filetest.o $(LIBS)
	$(CC) $(CFLAGS) filetest.o $(LIBS) -o $@

arosshell: arosshell.o $(LIBS)
	$(CC) $(CFLAGS) arosshell.o $(LIBS) -o $@

apps:
	@cd c; make \
	    TOP=".." CURDIR="$(CURDIR)/c" \
	    CC="$(CC)" COMMON_CFLAGS="$(COMMON_CFLAGS)" \
	    all

subdirs:
	@for dir in $(KERNEL) exec dos utility filesys libs ; do \
	    ( echo "Entering $$dir..." ; cd $$dir ; \
	    $(MAKE) \
		TOP=".." CURDIR="$(CURDIR)/$$dir" \
		CC="$(CC)" COMMON_CFLAGS="$(COMMON_CFLAGS)" \
		RM="$(RM)" \
		all ) ; \
	done

includes: include/aros/config/gnuc/exec_defines.h \
	include/aros/config/gnuc/dos_defines.h \
	include/aros/config/gnuc/utility_defines.h

include/aros/config/gnuc/exec_defines.h: m68k-native/*.c m68k-native/*.s exec/*.c dummy/*
	echo "#ifndef GCC_EXEC_DEFINES_H" >$@
	echo "#define GCC_EXEC_DEFINES_H" >>$@
	echo "#include <aros/config/gnuc/libcall_m68k.h>" >>$@
	echo "struct ExecBase;" >>$@
	echo "extern struct ExecBase *SysBase;" >>$@
	echo "#define Cause(a)" >>$@
	echo "" >>$@
	gawk -f gendef.awk $^ >>$@
	echo "#endif" >>$@

include/aros/config/gnuc/dos_defines.h: dos/*.c
	echo "#ifndef GCC_DOS_DEFINES_H" >$@
	echo "#define GCC_DOS_DEFINES_H" >>$@
	echo "#include <aros/config/gnuc/libcall_m68k.h>" >>$@
	echo "struct DosLibrary;" >>$@
	echo "extern struct DosLibrary *DOSBase;" >>$@
	echo "" >>$@
	gawk -f gendef.awk $^ >>$@
	echo "#endif" >>$@

include/aros/config/gnuc/utility_defines.h: utility/*.c
	echo "#ifndef GCC_UTILITY_DEFINES_H" >$@
	echo "#define GCC_UTILITY_DEFINES_H" >>$@
	echo "#include <aros/config/gnuc/libcall_m68k.h>" >>$@
	echo "struct UtilityBase;" >>$@
	echo "extern struct UtilityBase *UtilityBase;" >>$@
	echo "" >>$@
	gawk -f gendef.awk $^ >>$@
	echo "#endif" >>$@
