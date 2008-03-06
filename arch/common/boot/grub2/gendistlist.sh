#! /bin/sh
#
# Copyright (C) 2005  Free Software Foundation, Inc.
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
	gendistlist.sh genfslist.sh genkernsyms.sh genmk.rb \
	genmodsrc.sh gensymlist.sh install-sh mkinstalldirs stamp-h.in"

DISTDIRS="boot commands conf disk font fs hello include io kern loader \
	normal partmap term util video"

for f in $EXTRA_DISTFILES; do
    echo $f
done

dir=`dirname $0`
cd $dir

for dir in $DISTDIRS; do
  for d in `find $dir -type d | sort`; do
    find $d -maxdepth 1 -name '*.[chS]' -o -name '*.mk' -o -name '*.rmk' \
      -o -name '*.rb' -o -name '*.in' \
      | sort
  done
done
