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
	alib filesys libs devs c
DIST_FILES = makefile arosshell.c README.CVS make.cfg crypt.c \
	configure scripts/cint2.awk scripts/makefunctable.awk \
	scripts/genprotos.h

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
		$(addprefix AROS/, $(SUBDIRS) $(DIST_FILES)) \
		$(shell cd ..; find AROS/include -name "*.h")

dist-lha : FORCE
	cd bin/$(ARCH) ; \
	    lha a ../../dist/AROSbin-$(VERSION).lha AROS
	cd .. ; lha a AROS/dist/AROSdev-$(VERSION).lha \
		$(addprefix AROS/, $(SUBDIRS) $(DIST_FILES)) \
		$(shell cd ..; find AROS/include -name "*.h")

# Alwaye remake rules that depend on this one
FORCE :

setup :
	@if [ ! -d bin ]; then $(MKDIR) bin ; fi
	@if [ ! -d bin/$(ARCH) ]; then $(MKDIR) bin/$(ARCH) ; fi
	@if [ ! -d $(BINDIR) ]; then $(MKDIR) $(BINDIR) ; fi
	@if [ ! -d $(SDIR) ]; then $(MKDIR) $(SDIR) ; fi
	@if [ ! -d $(EXEDIR) ]; then $(MKDIR) $(EXEDIR) ; fi
	@if [ ! -d $(LIBDIR) ]; then $(MKDIR) $(LIBDIR) ; fi
	@if [ ! -d $(DEVSDIR) ]; then $(MKDIR) $(DEVSDIR) ; fi
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
	$(CC) $(CFLAGS) $< $(LIBS) $(X11LDFLAGS) -lX11 -o $@

subdirs:
	@for dir in $(SUBDIRS) ; do \
	    echo "Making all in $$dir..." ; \
	    if ( cd $$dir ; \
		$(MAKE) $(MFLAGS) CURDIR="$(CURDIR)/$$dir" all ) ; \
	    then echo -n ; else exit 1 ; fi \
	done

# I have to restart make here since not all files might be existing
# in $(OSGENDIR) at the time when make was started in the first place.
AmigaOS :
	$(MAKE) $(MFLAGS) $(LIBDIR)/libAmigaOS.a

$(LIBDIR)/libAmigaOS.a : $(wildcard $(OSGENDIR)/*.o) \
	    $(wildcard $(GENDIR)/alib/*.o)
	$(AR) $@ $?
	$(RANLIB) $@

$(SDIR)/Startup-Sequence : s/Startup-Sequence
	$(CP) $^ $@

includes: include/clib/exec_protos.h \
	    include/clib/dos_protos.h \
	    include/clib/utility_protos.h \
	    include/clib/graphics_protos.h \
	    include/clib/intuition_protos.h \
	    include/clib/console_protos.h

include/clib/exec_protos.h: $(wildcard $(KERNEL)/*.s $(KERNEL)/*.c exec/*.c)
	gawk -f scripts/genprotos.h --assign lib=Exec \
	$(KERNEL)/*.s $(KERNEL)/*.c exec/*.c

include/clib/dos_protos.h: $(wildcard dos/*.c)
	gawk -f scripts/genprotos.h --assign lib=Dos \
	dos/*.c

include/clib/utility_protos.h: $(wildcard utility/*.c)
	gawk -f scripts/genprotos.h --assign lib=Utility \
	utility/*.c

include/clib/graphics_protos.h: $(wildcard graphics/*.c)
	gawk -f scripts/genprotos.h --assign lib=Graphics \
	graphics/*.c

include/clib/intuition_protos.h: $(wildcard intuition/*.c)
	gawk -f scripts/genprotos.h --assign lib=Intuition \
	intuition/*.c

include/clib/console_protos.h: devs/cdinputhandler.c devs/rawkeyconvert.c
	gawk -f scripts/genprotos.h --assign lib=Console \
	devs/cdinputhandler.c devs/rawkeyconvert.c

.FORCE:

$(GENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@

#$(GENDIR)/%.d: %.c
#	 @$(RM) $@
#	 @touch $@
#	 @$(MKDEPEND) -f$@ -p$(GENDIR)/ -- $(CFLAGS) -- $^
#
#include $(GENDIR)/arosshell.d
