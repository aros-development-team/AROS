TOP=.
CURDIR=.

SPECIAL_CFLAGS = -Dmain=submain

include $(TOP)/make.cfg

DEP_LIBS= $(LIBDIR)/libAmigaOS.a \
    $(GENDIR)/filesys/emul_handler.o \
    $(LIBDIR)/libaros.a

LIBS=-L$(LIBDIR) \
	$(GENDIR)/filesys/emul_handler.o -lAmigaOS -laros

SUBDIRS = $(KERNEL) aros exec dos utility graphics intuition \
	filesys libs c
DIST_FILES = makefile arosshell.c README.CVS scripts/gendef.awk make.cfg \
	configure

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

all : setup subdirs AmigaOS \
	    $(BINDIR)/s/Startup-Sequence $(BINDIR)/arosshell

crypt : crypt.c
	$(CC) -o crypt crypt.c

dist : dist-dir dist-tar dist-lha
	cp README dist/AROSbin-$(VERSION).readme
	cp README dist/AROSdev-$(VERSION).readme

dist-dir : FORCE
	@if [ ! -d dist ]; then $(MKDIR) dist ; fi

dist-tar : FORCE
	cd bin/$(ARCH) ; \
	    tar cvvzf ../../dist/AROSbin-$(VERSION).tgz AROS
	cd .. ; tar cvvzf AROS/dist/AROSdev-$(VERSION).tgz \
		$(addprefix AROS/, $(SUBDIRS) $(DIST_FILES))

dist-lha : FORCE
	cd bin/$(ARCH) ; \
	    lha a ../../dist/AROSbin-$(VERSION).lha AROS
	cd .. ; lha a AROS/dist/AROSdev-$(VERSION).lha \
		$(addprefix AROS/, $(SUBDIRS) $(DIST_FILES))

# Alwaye remake rules that depend on this one
FORCE :

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
	$(RM) $(ARCHDIR) host.cfg
	@for dir in $(SUBDIRS) ; do \
	    ( echo "Cleaning in $$dir..." ; cd $$dir ; \
	    $(MAKE) $(MFLAGS) clean ) ; \
	done

$(BINDIR)/arosshell: $(GENDIR)/arosshell.o $(DEP_LIBS)
	$(CC) $(CFLAGS) $< $(LIBS) -L/usr/lib/X11 -lX11 -o $@

subdirs:
	@for dir in $(SUBDIRS) ; do \
	    echo "Making all in $$dir..." ; \
	    if ( cd $$dir ; \
		$(MAKE) $(MFLAGS) \
		    TOP=".." CURDIR="$(CURDIR)/$$dir" ARCH=$(ARCH) \
		    CC="$(CC)" COMMON_CFLAGS="$(COMMON_CFLAGS)" \
		    RM="$(RM)" \
		    all ) ; \
	    then echo -n ; else exit 1 ; fi \
	done

# I have to restart make here since not all files might be existing
# in $(OSGENDIR) at the time when make was started in the first place.
AmigaOS :
	$(MAKE) $(MFLAGS) $(LIBDIR)/libAmigaOS.a

$(LIBDIR)/libAmigaOS.a : $(wildcard $(OSGENDIR)/*.o)
	$(AR) $@ $^
	$(RANLIB) $@

$(SDIR)/Startup-Sequence : s/Startup-Sequence
	$(CP) $^ $@

includes: include/clib/exec_protos.h \
	    include/clib/dos_protos.h \
	    include/clib/utility_protos.h \
	    include/clib/graphics_protos.h \
	    include/clib/intuition_protos.h

include/clib/exec_protos.h: .FORCE
	gawk -f scripts/genprotos.h --assign lib=Exec \
	$(KERNEL)/*.s $(KERNEL)/*.c exec/*.c

include/clib/dos_protos.h: .FORCE
	gawk -f scripts/genprotos.h --assign lib=Dos \
	dos/*.c

include/clib/utility_protos.h: .FORCE
	gawk -f scripts/genprotos.h --assign lib=Utility \
	utility/*.c

include/clib/graphics_protos.h: .FORCE
	gawk -f scripts/genprotos.h --assign lib=Graphics \
	graphics/*.c

include/clib/intuition_protos.h: .FORCE
	gawk -f scripts/genprotos.h --assign lib=Intuition \
	intuition/*.c

.FORCE:

$(GENDIR)/test/%.o: test/%.c
	$(CC) $(CFLAGS) -I ./libs $^ -c -o $@

$(GENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@

$(TESTDIR)/devicetest: $(GENDIR)/test/devicetest.o $(GENDIR)/test/dummydev.o $(DEP_LIBS)
	$(CC) $(CFLAGS) $(GENDIR)/test/devicetest.o $(GENDIR)/test/dummydev.o $(LIBS) -o $@

$(TESTDIR)/% : $(GENDIR)/test/%.o $(DEP_LIBS)
	$(CC) $(CFLAGS) $< $(LIBS) -o $@

