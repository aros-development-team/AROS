# $Id$

# BEGIN_DESC{makefile}
# This is the toplevel makefile. Use it if you want to compile the whole
# distribution.
# END_DESC{makefile}

# BEGIN_DESC{makevar}
# \item{TOP} contains is the complete path to the root directory of the
#	project.
#
# \item{CURDIR} is the path from $(TOP) to the current directory.
#
# END_DESC{makevar}
TOP=.
CURDIR=.

# BEGIN_DESC{localmakevar}
# END_DESC{localmakevar}
SPECIAL_CFLAGS = -Dmain=submain

include $(TOP)/config/make.cfg

# BEGIN_DESC{localmakevar}
# \item{LIBS} Flags which are passed to the linker for executables.
#
# \item{DEP_LIBS} Files on which executables depend (ie. if these files are
#	newer than the executable, then the executable is linked anew).
#
# END_DESC{localmakevar}
ifeq ("$(SHARED_AR)","")
LIBAMIGAOS = $(LIBDIR)/libAmigaOS.a
else
LIBAMIGAOS = $(LIBDIR)/libAmigaOS.so
endif

DEP_LIBS= $(LIBAMIGAOS) \
    $(GENDIR)/filesys/emul_handler.o \
    $(LIBDIR)/libaros.a

LIBS=-L$(LIBDIR) \
	$(GENDIR)/filesys/emul_handler.o -lAmigaOS -laros

# BEGIN_DESC{localmakevar}
# \item{SUBDIRS} Contains the names of directories in which Make will recurse
#	for the main targets (eg. all, clean).
#
# END_DESC{localmakevar}
SUBDIRS = config rom compiler workbench apps

# BEGIN_DESC{localmakevar}
# \item{TESTDIR} The director in which the test cases will be put in
#
# \item{TESTS} Names of the files of the test cases
#
# END_DESC{localmakevar}
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

# BEGIN_DESC{target}
# \item{all} Compile the whole project (except the documentation).
#
# END_DESC{target}
ifeq ($(FLAVOUR),native)
all: setup subdirs
else
all : setup subdirs AmigaOS $(BINDIR)/arosshell
endif

# BEGIN_DESC{target}
# \item{crypt} Create the file crypt to create a password for CVS access
#
# END_DESC{target}
crypt : crypt.c
	$(CC) -o crypt crypt.c

# BEGIN_DESC{localmakevar}
# \item{BINARCHIVE} Basename of the binary archive
#
# \item{DEVARCHIVE} Basename of the source archive
#
# END_DESC{localmakevar}
BINARCHIVE = AROS-$(ARCH)-$(KERNEL)-$(VERSION)
DEVARCHIVE = AROSdev-$(VERSION)

# BEGIN_DESC{target}
# \item{dist} Create the distribution archives
#
# END_DESC{target}
# BEGIN_DESC{internaltarget}
# \item{dir-dir} Creates the directory for the distribution archives
#
# \item{dist-tar} Create .tar.gz archive of the binary for the local
#	architecture.
#
# \item{dist-lha} Create LhA archive of the binary for the local
#	architecture.
#
# \item{dist-src} Create the source archive as .tar.gz and LhA files.
#
# END_DESC{internaltarget}
dist : dist-dir dist-tar dist-lha dist-src
	cp README dist/$(BINARCHIVE).readme
	cp README dist/$(DEVARCHIVE).readme

dist-dir : FORCE
	@if [ ! -d dist ]; then $(MKDIR) dist ; else true ; fi
	@echo "Correcting access flags"
	@chmod -R ug=rwX,o=rX .

dist-tar : FORCE
	cd $(ARCHDIR) ; \
	    $(RM) ../../dist/$(BINARCHIVE).tgz ; \
	    tar chvvzf ../../dist/$(BINARCHIVE).tgz AROS

dist-lha : FORCE
	cd $(ARCHDIR) ; \
	    $(RM) ../../dist/$(BINARCHIVE).lha ; \
	    lha a ../../dist/$(BINARCHIVE).lha AROS

dist-src : FORCE
	$(TOP)/scripts/makedist src $(DEVARCHIVE)

# BEGIN_DESC{internaltarget}
# \item{FORCE} Alwaye remake rules that depend on this one
#
# END_DESC{internaltarget}
FORCE :

# BEGIN_DESC{internaltarget}
# \item{setup} Check the setup and create all directories and files which
#	are initially neccessary to compile AROS.
#
# END_DESC{internaltarget}
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
	@if [ ! -d $(OSMODDIR) ]; then $(MKDIR) $(OSMODDIR) ; else true ; fi
	@if [ ! -d $(GENDIR) ]; then $(MKDIR) $(GENDIR) ; else true ; fi
	@if [ ! -d $(GENDIR)/test ]; then $(MKDIR) $(GENDIR)/test ; else true ; fi
	@if [ ! -d $(GENDIR)/filesys ]; then $(MKDIR) $(GENDIR)/filesys ; else true ; fi
	@$(MAKE) $(MFLAGS) subdirs TARGET=setup

# BEGIN_DESC{target}
# \item{check} Run tests to check if AROS runs ok on your system.
#
# END_DESC{target}
check : $(TESTS)
	@for test in $(TESTS) ; do \
	    echo "Running test `basename $$test`" ; $$test ; \
	done

# BEGIN_DESC{target}
# \item{clean} Remove all generated files
#
# END_DESC{target}
clean:
	-$(RM) $(ARCHDIR) host.cfg
	$(MAKE) $(MFLAGS) subdirs TARGET=clean

# BEGIN_DESC{internaltarget}
# \item{$(BINDIR)/arosshell} Create the AROS shell for systems which
#	support emulation.
#
# END_DESC{internaltarget}
$(BINDIR)/arosshell: $(GENDIR)/arosshell.o $(DEP_LIBS)
	$(CC) $(CFLAGS) $< $(LIBS) $(GUI_LDFLAGS) $(GUI_LIBFLAGS) -o $@

# BEGIN_DESC{internaltarget}
# \item{subdirs} Pass global targets to all subdirectories.
#
# END_DESC{internaltarget}
subdirs:
	@for dir in $(SUBDIRS) ; do \
	    echo "Making $(TARGET) in $$dir..." ; \
	    if ( cd $$dir ; \
		$(MAKE) $(MFLAGS) TOP=".." CURDIR="$(CURDIR)/$$dir" \
		TARGET=$(TARGET) $(TARGET) ) ; \
	    then true ; else exit 1 ; fi ; \
	done

# BEGIN_DESC{internaltarget}
# \item{AmigaOS} I have to restart make here since not all files might be
#	existing in $(OSGENDIR) at the time when make was started in the
#	first place.
#
# END_DESC{internaltarget}
AmigaOS :
	@$(MAKE) $(MFLAGS) $(LIBAMIGAOS)

# BEGIN_DESC{internaltarget}
# \item{$(LIBDIR)/libAmigaOS.a} Recreate the kernel if any kernel function
#	has been recompiled.
#
# END_DESC{internaltarget}
$(LIBAMIGAOS) : $(wildcard $(OSGENDIR)/*.o) \
	    $(wildcard $(GENDIR)/alib/*.o)
	@echo "Recreating library"
ifeq ("$(SHARED_AR)","")
	@$(AR) $@ $?
	$(RANLIB) $@
else
	@$(SHARED_AR) $@ $^
endif

CLIBDIR=$(TOP)/compiler/include/clib

includes: \
	    $(CLIBDIR)/exec_protos.h \
	    $(CLIBDIR)/dos_protos.h \
	    $(CLIBDIR)/utility_protos.h \
	    $(CLIBDIR)/graphics_protos.h \
	    $(CLIBDIR)/intuition_protos.h \
	    $(CLIBDIR)/console_protos.h

$(CLIBDIR)/exec_protos.h: $(wildcard config/$(KERNEL)/*.s \
	    config/$(KERNEL)/*.c rom/exec/*.c) scripts/genprotos.h
	gawk -f scripts/genprotos.h --assign lib=Exec \
	config/$(KERNEL)/*.s config/$(KERNEL)/*.c rom/exec/*.c

$(CLIBDIR)/dos_protos.h: $(wildcard rom/dos/*.c) scripts/genprotos.h
	gawk -f scripts/genprotos.h --assign lib=Dos \
	rom/dos/*.c

$(CLIBDIR)/utility_protos.h: $(wildcard rom/utility/*.c) scripts/genprotos.h
	gawk -f scripts/genprotos.h --assign lib=Utility \
	rom/utility/*.c

$(CLIBDIR)/graphics_protos.h: $(wildcard rom/graphics/*.c) scripts/genprotos.h
	gawk -f scripts/genprotos.h --assign lib=Graphics \
	rom/graphics/*.c

$(CLIBDIR)/intuition_protos.h: $(wildcard rom/intuition/*.c) scripts/genprotos.h
	gawk -f scripts/genprotos.h --assign lib=Intuition \
	rom/intuition/*.c

$(CLIBDIR)/console_protos.h: rom/devs/cdinputhandler.c \
	    rom/devs/rawkeyconvert.c scripts/genprotos.h
	gawk -f scripts/genprotos.h --assign lib=Console \
	rom/devs/cdinputhandler.c rom/devs/rawkeyconvert.c

$(GENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@

#$(GENDIR)/%.d: %.c
#	 @$(RM) $@
#	 @touch $@
#	 @$(MKDEPEND) -f$@ -p$(GENDIR)/ -- $(CFLAGS) -- $^
#
#include $(GENDIR)/arosshell.d

# BEGIN_DESC{target}
# \item{cleandep} Remove all generated dependency files.
#
# END_DESC{target}
cleandep:
	find $(GENDIR) -name "*.d" -exec $(RM) "{}" \;
