#!/bin/sh

############################################################################
#
# BetterString.mcc - A better String gadget MUI Custom Class
# Copyright (C) 1997-2000 Allan Odgaard
# Copyright (C) 2005-2009 by BetterString.mcc Open Source Team
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# BetterString class Support Site:  http://www.sf.net/projects/bstring-mcc/
#
# $Id$
#
############################################################################

# BetterString.mcc release build script
# invoke this script as "./mkrelease.sh" to build the release archives

rm -rf "release"
mkdir -p "release"
mkdir -p "release/codesets"
mkdir -p "release/codesets/Charsets"
mkdir -p "release/codesets/Demos"
mkdir -p "release/codesets/Developer"
mkdir -p "release/codesets/Developer/Autodocs"
mkdir -p "release/codesets/Developer/Examples"
mkdir -p "release/codesets/Developer/fd"
mkdir -p "release/codesets/Developer/include"
mkdir -p "release/codesets/Developer/sfd"
mkdir -p "release/codesets/Developer/xml"
mkdir -p "release/codesets/Libs"

make -C src release
make -C developer/examples release

for os in os3 os4 mos aros-i386 aros-ppc aros-x86_64; do
	case $os in
	    os3)         fullsys="AmigaOS3";;
	    os4)         fullsys="AmigaOS4";;
	    mos)         fullsys="MorphOS";;
	    aros-i386)   fullsys="AROS-i386";;
	    aros-ppc)    fullsys="AROS-ppc";;
	    aros-x86_64) fullsys="AROS-x86_64";;
	esac
	mkdir -p "release/codesets/Demos/$fullsys"
	for demo in b64d b64e Convert demo1 DetectCodeset UTF8ToStrHook; do
		cp -a developer/examples/bin_$os/$demo "release/codesets/Demos/$fullsys/"
	done
	mkdir -p "release/codesets/Libs/$fullsys"
	cp -a src/bin_$os/codesets.library "release/codesets/Libs/$fullsys/"
done

cp -a -R dist/* "release/"
cp -a AUTHORS ChangeLog COPYING "release/codesets/"
cp -a charsets/* "release/codesets/Charsets/"
cp -a developer/docs/* "release/codesets/Developer/Autodocs/"
cp -a developer/examples/*.c "release/codesets/Developer/Examples/"
cp -a developer/examples/Makefile "release/codesets/Developer/Examples/"
cp -a developer/examples/mmakefile.src "release/codesets/Developer/Examples/"
cp -a developer/fd/* "release/codesets/Developer/fd/"
cp -a -R developer/include/* "release/codesets/Developer/include/"
cp -a developer/sfd/* "release/codesets/Developer/sfd/"
cp -a developer/xml/* "release/codesets/Developer/xml/"

releasever=`grep "#define LIB_VERSION" src/version.h | awk '{ print $3 }'`
releaserev=`grep "#define LIB_REVISION" src/version.h | awk '{ print $3 }'`

echo "  MK codesets-$releasever.$releaserev.lha"
find release -nowarn -name ".svn" -exec rm -rf {} \; 2>/dev/null
find release -nowarn -name "mmakefile.src" -exec rm -rf {} \; 2>/dev/null
cd release
lha -ao5q ../codesets-$releasever.$releaserev.lha *
cp codesets/ReadMe ../codesets-$releasever.$releaserev.readme
cd ..
