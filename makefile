ARCH=linux
KERNEL=i386-emul

TOP=.
CURDIR=.

SPECIAL_CFLAGS = -Dmain=submain

include $(TOP)/make.cfg

DEP_LIBS=$(LIBDIR)/libkernel.a $(LIBDIR)/libexec.a $(LIBDIR)/libkernel.a \
    $(LIBDIR)/libdos.a $(OBJDIR)/filesys/emul_handler.o \
    $(LIBDIR)/libutility.a $(LIBDIR)/libaros.a

LIBS=-L$(LIBDIR) -lkernel \
	$(OBJDIR)/filesys/emul_handler.o \
	-lutility -ldos -lexec -laros -lkernel

TESTDIR = $(BINDIR)/test
TESTS = $(TESTDIR)/tasktest \
	$(TESTDIR)/signaltest \
	$(TESTDIR)/exceptiontest \
	$(TESTDIR)/tasktest2 \
	$(TESTDIR)/messagetest \
	$(TESTDIR)/semaphoretest \
	$(TESTDIR)/initstructtest \
	$(TESTDIR)/devicetest \
	$(TESTDIR)/filetest

all : setup subdirs $(BINDIR)/arosshell apps

crypt : crypt.c
	$(CC) -o crypt crypt.c

setup :
	@echo "Setting up"
	@if [ ! -d bin ]; then $(MKDIR) bin ; fi
	@if [ ! -d $(BINDIR) ]; then $(MKDIR) $(BINDIR) ; fi
	@if [ ! -d $(EXEDIR) ]; then $(MKDIR) $(EXEDIR) ; fi
	@if [ ! -d $(LIBDIR) ]; then $(MKDIR) $(LIBDIR) ; fi
	@if [ ! -d $(SLIBDIR) ]; then $(MKDIR) $(SLIBDIR) ; fi
	@if [ ! -d $(TESTDIR) ]; then $(MKDIR) $(TESTDIR) ; fi
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; fi
	@if [ ! -d $(OBJDIR)/test ]; then $(MKDIR) $(OBJDIR)/test ; fi
	@if [ ! -d $(OBJDIR)/filesys ]; then $(MKDIR) $(OBJDIR)/filesys ; fi
	$(CP) s $(BINDIR)
	@touch setup

check : $(TESTS)
	@for test in $(TESTS) ; do \
	    echo "Running test `basename $$test`" ; $$test ; \
	done

clean:
	$(RM) $(BINDIR) setup
	@for dir in $(KERNEL) exec dos utility filesys libs ; do \
	    ( cd $$dir ; \
	    $(MAKE) $(MFLAGS) clean ) ; \
	done

$(BINDIR)/arosshell: $(OBJDIR)/arosshell.o $(DEP_LIBS)
	$(CC) $(CFLAGS) $< $(LIBS) -o $@

apps:
	@cd c; $(MAKE) $(MFLAGS) \
	    TOP=".." CURDIR="$(CURDIR)/c" ARCH=$(ARCH) \
	    CC="$(CC)" COMMON_CFLAGS="$(COMMON_CFLAGS)" \
	    all

subdirs:
	@for dir in $(KERNEL) aros exec dos utility filesys libs ; do \
	    ( echo "Entering $$dir..." ; cd $$dir ; \
	    $(MAKE) $(MFLAGS) \
		TOP=".." CURDIR="$(CURDIR)/$$dir" ARCH=$(ARCH) \
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

$(OBJDIR)/test/%.o: test/%.c
	$(CC) $(CFLAGS) -I ./libs $^ -c -o $@

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@

$(TESTDIR)/devicetest: $(OBJDIR)/test/devicetest.o $(OBJDIR)/test/dummydev.o $(DEP_LIBS)
	$(CC) $(CFLAGS) $(OBJDIR)/test/devicetest.o $(OBJDIR)/test/dummydev.o $(LIBS) -o $@

$(TESTDIR)/% : $(OBJDIR)/test/%.o $(DEP_LIBS)
	$(CC) $(CFLAGS) $< $(LIBS) -o $@

