#!/bin/sh

############################################################################
#
# $Id$
#
# Copyright (C) 1993-1999 by Jochen Wiedmann and Marcin Orlowski
# Copyright (C) 2002-2007 by the FlexCat Open Source Team
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or (at
# your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
#
############################################################################

# FlexCat release build script
# invoke this script as "./mkrelease.sh" to build the release archives

rm -rf "release"
mkdir -p "release"
mkdir -p "release/FlexCat"
mkdir -p "release/FlexCat/Locale"
mkdir -p "release/FlexCat/Locale/Catalogs"
mkdir -p "release/FlexCat/Docs"
mkdir -p "release/FlexCat/Lib"
mkdir -p "release/FlexCat/Contribution"

make -C src release

exe="flexcat"

for os in os3 os4 mos aros-i386 aros-ppc aros-x86_64 unix mingw32; do
	case $os in
	    os3)         fullsys="AmigaOS3";;
	    os4)         fullsys="AmigaOS4";;
	    mos)         fullsys="MorphOS";;
	    aros-i386)   fullsys="AROS-i386";;
	    aros-ppc)    fullsys="AROS-ppc";;
	    aros-x86_64) fullsys="AROS-x86_64";;
	    unix)        fullsys="Linux-i386";;
      mingw32)     fullsys="Windows"; exe="flexcat.exe";;
	esac
	mkdir -p "release/FlexCat/$fullsys"
	cp -a src/bin_$os/$exe "release/FlexCat/$fullsys/"
done

make -C src catalogs

for language in `ls src/locale/*.catalog`; do
  catalog=$(basename "$language")
  lang="${catalog%.*}"
  mkdir -p "release/FlexCat/Locale/Catalogs/${lang}"
  cp -a ${language} "release/FlexCat/Locale/Catalogs/${lang}/FlexCat.catalog"
done

cp -a -R dist/* "release/"
cp -a src/locale/FlexCat.pot "release/FlexCat/Locale/"
cp -a src/sd/* "release/FlexCat/Lib/"
cp -a doc/FlexCat.readme "release/FlexCat/"
cp -a AUTHORS ChangeLog COPYING "release/FlexCat/"
cp -a contrib/* "release/FlexCat/Contribution/"

releasever=`grep "#define EXE_VERSION" src/version.h | awk '{ print $3 }'`
releaserev=`grep "#define EXE_REVISION" src/version.h | awk '{ print $3 }'`

echo "  MK FlexCat-$releasever.$releaserev.lha"
find release -nowarn -name ".svn" -or -name ".AppleDouble" -exec rm -rf {} \; 2>/dev/null
cd release
rm -f ../FlexCat-$releasever.$releaserev.lha
lha -ao5q ../FlexCat-$releasever.$releaserev.lha *
cp FlexCat/FlexCat.readme ../FlexCat-$releasever.$releaserev.readme
cd ..
