# $Id$
TOP=.
CURDIR=.

SPECIAL_CFLAGS = -Dmain=submain

include $(TOP)/config/make.cfg

DEP_LIBS= $(LIBDIR)/libAmigaOS.a \
    $(GENDIR)/filesys/emul_handler.o \
    $(LIBDIR)/libaros.a

LIBS=-L$(LIBDIR) \
	$(GENDIR)/filesys/emul_handler.o -lAmigaOS -laros

ifeq ($(FLAVOUR),native)
# Only these subdirs for the native Amiga version, for the moment.
SUBDIRS = config \
	include clib exec
else
SUBDIRS = config rom apps/compiler workbench apps
endif

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

ifeq ($(FLAVOUR),native)
all: setup subdirs
else
all : setup subdirs AmigaOS $(BINDIR)/arosshell
endif

crypt : crypt.c
	$(CC) -o crypt crypt.c

BINARCHIVE = AROS-$(ARCH)-$(KERNEL)-$(VERSION)
DEVARCHIVE = AROSdev-$(VERSION)

dist : dist-dir dist-tar dist-lha dist-src
	cp README dist/$(BINARCHIVE).readme
	cp README dist/$(DEVARCHIVE).readme

dist-dir : FORCE
	@if [ ! -d dist ]; then $(MKDIR) dist ; else true ; fi

dist-tar : FORCE
	cd $(ARCHDIR) ; \
	    $(RM) ../../dist/$(BINARCHIVE).tgz ; \
	    tar chvzf ../../dist/$(BINARCHIVE).tgz AROS

dist-lha : FORCE
	cd $(ARCHDIR) ; \
	    $(RM) ../../dist/$(BINARCHIVE).lha ; \
	    lha a ../../dist/$(BINARCHIVE).lha AROS

dist-src : FORCE
	$(TOP)/scripts/makedist src $(DEVARCHIVE)

# Alwaye remake rules that depend on this one
FORCE :

setup :
	@if [ ! -d amiga/include ]; then \
	    echo "Missing AmigaOS includes. Please get a copy and put" ; \
	    echo "them into amiga/include." ; \
	    exit 10 ; \
	else true ; fi
	@if [ ! -d bin ]; then $(MKDIR) bin ; else true ; fi
	@if [ ! -d $(ARCHDIR) ]; then $(MKDIR) $(ARCHDIR) ; else true ; fi
	@if [ ! -d $(BINDIR) ]; then $(MKDIR) $(BINDIR) ; else true ; fi
	@if [ ! -d $(SDIR) ]; then $(MKDIR) $(SDIR) ; else true ; fi
	@if [ ! -d $(EXEDIR) ]; then $(MKDIR) $(EXEDIR) ; else true ; fi
	@if [ ! -d $(LIBDIR) ]; then $(MKDIR) $(LIBDIR) ; else true ; fi
	@if [ ! -d $(DEVSDIR) ]; then $(MKDIR) $(DEVSDIR) ; else true ; fi
	@if [ ! -d $(SLIBDIR) ]; then $(MKDIR) $(SLIBDIR) ; else true ; fi
	@if [ ! -d $(TESTDIR) ]; then $(MKDIR) $(TESTDIR) ; else true ; fi
	@if [ ! -d $(GENDIR) ]; then $(MKDIR) $(GENDIR) ; else true ; fi
	@if [ ! -d $(GENDIR)/test ]; then $(MKDIR) $(GENDIR)/test ; else true ; fi
	@if [ ! -d $(GENDIR)/filesys ]; then $(MKDIR) $(GENDIR)/filesys ; else true ; fi
	@cd apps/compiler/include ; \
	$(MAKE) $(MFLAGS) TOP="../../.." CURDIR="$(CURDIR)/apps/compiler/include" \
		all

check : $(TESTS)
	@for test in $(TESTS) ; do \
	    echo "Running test `basename $$test`" ; $$test ; \
	done

clean:
	$(RM) $(ARCHDIR) host.cfg
	@for dir in $(SUBDIRS) ; do \
	    ( echo "Cleaning in $$dir..." ;
	      if [ "$$dir" = "apps/compiler" ]; then \
		  top="../.." ; else top=".." ; \
	      fi ; \
	      cd $$dir ; \
	      $(MAKE) $(MFLAGS) TOP="$$top" CURDIR="$(CURDIR)/$$dir" \
	      clean ) ; \
	done

$(BINDIR)/arosshell: $(GENDIR)/arosshell.o $(DEP_LIBS)
	$(CC) $(CFLAGS) $< $(LIBS) $(GUI_LDFLAGS) $(GUI_LIBFLAGS) -o $@

subdirs:
	@for dir in $(SUBDIRS) ; do \
	    echo "Making all in $$dir..." ; \
	    if [ "$$dir" = "apps/compiler" ]; then \
		top="../.." ; else top=".." ; \
	    fi ; \
	    if ( cd $$dir ; \
		$(MAKE) $(MFLAGS) TOP="$$top" CURDIR="$(CURDIR)/$$dir" \
		all ) ; \
	    then echo -n ; else exit 1 ; fi ; \
	done

# I have to restart make here since not all files might be existing
# in $(OSGENDIR) at the time when make was started in the first place.
AmigaOS :
	$(MAKE) $(MFLAGS) $(LIBDIR)/libAmigaOS.a

$(LIBDIR)/libAmigaOS.a : $(wildcard $(OSGENDIR)/*.o) \
	    $(wildcard $(GENDIR)/alib/*.o)
	$(AR) $@ $?
	$(RANLIB) $@

# include/clib/exec_protos.h
includes: \
	    apps/compiler/include/clib/dos_protos.h \
	    apps/compiler/include/clib/utility_protos.h \
	    apps/compiler/include/clib/graphics_protos.h \
	    apps/compiler/include/clib/intuition_protos.h \
	    apps/compiler/include/clib/console_protos.h

apps/compiler/include/clib/exec_protos.h: $(wildcard $(KERNEL)/*.s $(KERNEL)/*.c exec/*.c)
	gawk -f scripts/genprotos.h --assign lib=Exec \
	config/$(KERNEL)/*.s config/$(KERNEL)/*.c exec/*.c

apps/compiler/include/clib/dos_protos.h: $(wildcard dos/*.c)
	gawk -f scripts/genprotos.h --assign lib=Dos \
	dos/*.c

apps/compiler/include/clib/utility_protos.h: $(wildcard utility/*.c)
	gawk -f scripts/genprotos.h --assign lib=Utility \
	utility/*.c

apps/compiler/include/clib/graphics_protos.h: $(wildcard graphics/*.c)
	gawk -f scripts/genprotos.h --assign lib=Graphics \
	graphics/*.c

apps/compiler/include/clib/intuition_protos.h: $(wildcard intuition/*.c)
	gawk -f scripts/genprotos.h --assign lib=Intuition \
	intuition/*.c

apps/compiler/include/clib/console_protos.h: devs/cdinputhandler.c devs/rawkeyconvert.c
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

cleandep:
	$(RM) $(GENDIR)/*.d $(GENDIR)/*/*.d
# DO NOT DELETE THIS LINE -- make depend depends on it.
