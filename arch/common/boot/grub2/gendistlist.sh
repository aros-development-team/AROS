#! /bin/sh
#
# Copyright (C) 2005, 2008, 2009  Free Software Foundation, Inc.
#
# This gendistlist.sh is free software; the author
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

# Generate a list of distributed files.

EXTRA_DISTFILES="AUTHORS COPYING ChangeLog DISTLIST INSTALL NEWS README \
	THANKS TODO Makefile.in aclocal.m4 autogen.sh config.guess \
	config.h.in config.sub configure configure.ac gencmdlist.sh \
	gendistlist.sh genfslist.sh genhandlerlist.sh geninit.sh \
	geninitheader.sh genkernsyms.sh.in genmk.rb genmoddep.awk \
	genmodsrc.sh genpartmaplist.sh genparttoollist.sh \
	gensymlist.sh.in install-sh mkinstalldirs stamp-h.in"

DISTDIRS="boot bus commands conf disk docs efiemu font fs hello hook include io \
	kern lib loader mmap normal partmap parttool script term util video"

LC_COLLATE=C
export LC_COLLATE

for f in $EXTRA_DISTFILES; do
    echo $f
done

dir=`dirname $0`
cd $dir

for dir in $DISTDIRS; do
  for d in `find $dir -type d | sed '/\/\.svn$/d;\/\.svn\//d' | sort`; do
    find $d -maxdepth 1 -name '*.[chSy]' -o -name '*.mk' -o -name '*.rmk' \
      -o -name '*.rb' -o -name '*.in' -o -name '*.tex' -o -name '*.texi' \
      -o -name 'grub.cfg' -o -name 'README' -o -name '*.sc' -o -name 'mdate-sh' \
      -o -name '*.sh' | sort
  done
done
