ARCH=linux
KERNEL=i386-emul

VERSION = 1.5

TOP=.
CURDIR=.

SPECIAL_CFLAGS = -Dmain=submain

include $(TOP)/make.cfg

DEP_LIBS= $(LIBDIR)/libAmigaOS.a \
    $(GENDIR)/filesys/emul_handler.o \
    $(LIBDIR)/libaros.a

LIBS=-L$(LIBDIR) \
	$(GENDIR)/filesys/emul_handler.o -lAmigaOS -laros

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

all : setup subdirs $(LIBDIR)/libAmigaOS.a $(BINDIR)/s/Startup-Sequence \
	    $(BINDIR)/arosshell apps

crypt : crypt.c
	$(CC) -o crypt crypt.c

dodist : dist-dir dist-bin dist-src

dist-dir :
	@if [ ! -d dist ]; then $(MKDIR) dist ; fi

dist-bin :
	cd bin/$(ARCH) ; tar cvvzf ../../AROS_$(ARCH)_bin-$(VERSION).tar.gz \
		AROS

dist-src :
	tar cvvzf AROS-$(VERSION).tar.gz \
		README.CVS aros arosshell.c c crypt.c \
		devs dos dummy exec filesys gendef.awk \
		i386-emul include libs m68k-emul m68k-native \
		make.cfg makefile s test utility

setup :
	@if [ ! -d bin ]; then $(MKDIR) bin ; fi
	@if [ ! -d bin/$(ARCH) ]; then $(MKDIR) bin/$(ARCH) ; fi
	@if [ ! -d $(BINDIR) ]; then $(MKDIR) $(BINDIR) ; fi
	@if [ ! -d $(SDIR) ]; then $(MKDIR) $(SDIR) ; fi
	@if [ ! -d $(EXEDIR) ]; then $(MKDIR) $(EXEDIR) ; fi
	@if [ ! -d $(LIBDIR) ]; then $(MKDIR) $(LIBDIR) ; fi
	@if [ ! -d $(SLIBDIR) ]; then $(MKDIR) $(SLIBDIR) ; fi
	@if [ ! -d $(TESTDIR) ]; then $(MKDIR) $(TESTDIR) ; fi
	@if [ ! -d $(GENDIR) ]; then $(MKDIR) $(GENDIR) ; fi
	@if [ ! -d $(GENDIR)/test ]; then $(MKDIR) $(GENDIR)/test ; fi
	@if [ ! -d $(GENDIR)/filesys ]; then $(MKDIR) $(GENDIR)/filesys ; fi

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

$(BINDIR)/arosshell: $(GENDIR)/arosshell.o $(DEP_LIBS)
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

$(LIBDIR)/libAmigaOS.a : $(wildcard $(OSGENDIR)/*.o)
	$(AR) $@ $^
	$(RANLIB) $@

$(SDIR)/Startup-Sequence : s/Startup-Sequence
	$(CP) $^ $@

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

$(GENDIR)/test/%.o: test/%.c
	$(CC) $(CFLAGS) -I ./libs $^ -c -o $@

$(GENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@

$(TESTDIR)/devicetest: $(GENDIR)/test/devicetest.o $(GENDIR)/test/dummydev.o $(DEP_LIBS)
	$(CC) $(CFLAGS) $(GENDIR)/test/devicetest.o $(GENDIR)/test/dummydev.o $(LIBS) -o $@

$(TESTDIR)/% : $(GENDIR)/test/%.o $(DEP_LIBS)
	$(CC) $(CFLAGS) $< $(LIBS) -o $@

