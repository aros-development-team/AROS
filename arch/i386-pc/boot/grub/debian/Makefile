# Makefile.in generated automatically by automake 1.4a from Makefile.am

# Copyright (C) 1994, 1995-9, 2000 Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

SHELL = /bin/sh

srcdir = .
top_srcdir = ..
prefix = /usr/local
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
datadir = ${prefix}/share
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com
localstatedir = ${prefix}/var
libdir = ${exec_prefix}/lib
infodir = ${prefix}/info
mandir = ${prefix}/man
includedir = ${prefix}/include
oldincludedir = /usr/include

pkgdatadir = $(datadir)/grub
pkglibdir = $(libdir)/grub
pkgincludedir = $(includedir)/grub

top_builddir = ..

ACLOCAL = ${SHELL} /home/basti/Files/Linux/grub-0.5.96.1/missing --run aclocal
AUTOCONF = ${SHELL} /home/basti/Files/Linux/grub-0.5.96.1/missing --run autoconf
AUTOMAKE = ${SHELL} /home/basti/Files/Linux/grub-0.5.96.1/missing --run automake
AUTOHEADER = ${SHELL} /home/basti/Files/Linux/grub-0.5.96.1/missing --run autoheader

INSTALL = /usr/bin/ginstall -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL}
INSTALL_STRIP_FLAG =
transform = s,x,x,

NORMAL_INSTALL = :
PRE_INSTALL = :
POST_INSTALL = :
NORMAL_UNINSTALL = :
PRE_UNINSTALL = :
POST_UNINSTALL = :


host_alias = i686-pc-linux-gnu
host_triplet = i686-pc-linux-gnu
AMDEP = 
AMTAR = ${SHELL} /home/basti/Files/Linux/grub-0.5.96.1/missing --run tar
AWK = gawk
CC = gcc
CPP = gcc -E
CXX = @CXX@
CXXCPP = @CXXCPP@
DEPDIR = .deps
FSYS_CFLAGS =  -DFSYS_EXT2FS=1 -DFSYS_FAT=1 -DFSYS_FFS=1 -DFSYS_MINIX=1 -DFSYS_REISERFS=1 -DFSYS_AFFS=1
GRUB_CFLAGS = -O2
GRUB_LIBS =  -lncurses
MAINT = #
MAKEINFO = ${SHELL} /home/basti/Files/Linux/grub-0.5.96.1/missing --run makeinfo
NETBOOT_DRIVERS = 
NET_CFLAGS = 
NET_EXTRAFLAGS =  -DCONGESTED=1 -DNE_SCAN=0x280,0x300,0x320,0x340 -DWD_DEFAULT_MEM=0xCC000
OBJCOPY = objcopy
PACKAGE = grub
PERL = 
RANLIB = ranlib
STAGE1_CFLAGS = -O2
STAGE2_CFLAGS = -Os
VERSION = 0.5.96.1
host_cpu = i386
host_vendor = pc
install_sh = ./install-sh


EXTRA_DIST = README.debian changelog control copyright postinst prerm rules
subdir = debian
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_HEADER = ../config.h
CONFIG_CLEAN_FILES = 
DIST_SOURCES = 
DIST_COMMON =  Makefile.am Makefile.in


DISTFILES = $(DIST_COMMON) $(DIST_SOURCES) $(TEXINFOS) $(EXTRA_DIST)

GZIP_ENV = --best
all: all-redirect
.SUFFIXES:
$(srcdir)/Makefile.in: # Makefile.am $(top_srcdir)/configure.in $(ACLOCAL_M4) 
	cd $(top_srcdir) && $(AUTOMAKE) --gnu debian/Makefile

Makefile: $(srcdir)/Makefile.in  $(top_builddir)/config.status
	cd $(top_builddir) \
	  && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status

tags: TAGS
TAGS:


distdir = $(top_builddir)/$(PACKAGE)-$(VERSION)/$(subdir)

distdir: $(DISTFILES)
	@for file in $(DISTFILES); do \
	  d=$(srcdir); \
	  if test -d $$d/$$file; then \
	    cp -pR $$d/$$file $(distdir) \
	    || exit 1; \
	  else \
	    test -f $(distdir)/$$file \
	    || cp -p $$d/$$file $(distdir)/$$file \
	    || exit 1; \
	  fi; \
	done
info-am:
info: info-am
dvi-am:
dvi: dvi-am
check-am: all-am
check: check-am
installcheck-am:
installcheck: installcheck-am
install-exec-am:
install-exec: install-exec-am

install-data-am:
install-data: install-data-am

install-am: all-am
	@$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am
install: install-am
uninstall-am:
uninstall: uninstall-am
all-am: Makefile
all-redirect: all-am
install-strip:
	$(MAKE) $(AM_MAKEFLAGS) INSTALL_STRIP_FLAG=-s install
installdirs:


mostlyclean-generic:

clean-generic:

distclean-generic:
	-rm -f Makefile $(CONFIG_CLEAN_FILES)
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*

maintainer-clean-generic:
	-rm -f Makefile.in
mostlyclean-am:  mostlyclean-generic

mostlyclean: mostlyclean-am

clean-am:  clean-generic mostlyclean-am

clean: clean-am

distclean-am:  distclean-generic clean-am

distclean: distclean-am

maintainer-clean-am:  maintainer-clean-generic distclean-am
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

maintainer-clean: maintainer-clean-am

.PHONY: tags distdir info-am info dvi-am dvi check check-am \
installcheck-am installcheck install-exec-am install-exec \
install-data-am install-data install-am install uninstall-am uninstall \
all-redirect all-am all install-strip installdirs mostlyclean-generic \
distclean-generic clean-generic maintainer-clean-generic clean \
mostlyclean distclean maintainer-clean


# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
