ARCH=linux
KERNEL=i386-emul

TOP=.
CURDIR=.
COMMON_CFLAGS=-Wall -g
CFLAGS=$(COMMON_CFLAGS) -Dmain=submain -I $(TOP)/include -I $(TOP)/amiga/include
CC=gcc
RM=rm -rf
MKDIR=mkdir
TOUCH=touch

BINDIR=bin/$(ARCH)

LIBS=$(KERNEL)/libkernel.a exec/libexec.a $(KERNEL)/libkernel.a dos/libdos.a \
filesys/emul_handler.o utility/libutility.a

TESTS = $(BINDIR)/test/tasktest \
	$(BINDIR)/test/signaltest \
	$(BINDIR)/test/exceptiontest \
	$(BINDIR)/test/tasktest2 \
	$(BINDIR)/test/messagetest \
	$(BINDIR)/test/semaphoretest \
	$(BINDIR)/test/initstructtest \
	$(BINDIR)/test/devicetest \
	$(BINDIR)/test/filetest


obj/%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@

obj/test/%.o: test/%.c
	$(CC) $(CFLAGS) $^ -c -o $@

all : setup subdirs $(BINDIR)/arosshell apps

setup :
	@echo "Setting up"
	-@$(MKDIR) bin
	-@$(MKDIR) $(BINDIR)
	-@$(MKDIR) $(BINDIR)/c
	-@$(MKDIR) $(BINDIR)/lib
	-@$(MKDIR) $(BINDIR)/test
	-@$(MKDIR) obj
	-@$(MKDIR) obj/test
	@touch setup

test : $(TESTS)

clean:
	$(RM) $(TESTS) core
	@for dir in $(KERNEL) exec dos utility filesys libs ; do \
	    ( cd $$dir ; \
	    $(MAKE) clean ) ; \
	done

$(BINDIR)/arosshell: obj/arosshell.o $(LIBS)
	$(CC) $(CFLAGS) $< $(LIBS) -o $@

apps:
	@cd c; make \
	    TOP=".." CURDIR="$(CURDIR)/c" BINDIR="../$(BINDIR)/c" \
	    CC="$(CC)" COMMON_CFLAGS="$(COMMON_CFLAGS)" \
	    all

subdirs:
	@for dir in $(KERNEL) exec dos utility filesys libs ; do \
	    ( echo "Entering $$dir..." ; cd $$dir ; \
	    $(MAKE) \
		TOP=".." CURDIR="$(CURDIR)/c" BINDIR="$(BINDIR)" \
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

$(BINDIR)/test/% : obj/test/%.o
	$(CC) $(CFLAGS) $< $(LIBS) -o $@

