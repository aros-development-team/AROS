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
mkdir -p "release/MCC_BetterString"
mkdir -p "release/MCC_BetterString/Developer"
mkdir -p "release/MCC_BetterString/Developer/Autodocs"
mkdir -p "release/MCC_BetterString/Developer/C"
mkdir -p "release/MCC_BetterString/Developer/C/include"
mkdir -p "release/MCC_BetterString/Developer/C/include/mui"
mkdir -p "release/MCC_BetterString/Libs"
mkdir -p "release/MCC_BetterString/Libs/MUI"
mkdir -p "release/MCC_BetterString/Locale"
mkdir -p "release/MCC_BetterString/Locale/Catalogs"

make -C mcc release
make -C mcc/hotkeystring release
make -C mcp release

for os in os3 os4 mos aros-i386 aros-ppc aros-x86_64; do
	case $os in
	    os3)         fullsys="AmigaOS3";;
	    os4)         fullsys="AmigaOS4";;
	    mos)         fullsys="MorphOS";;
	    aros-i386)   fullsys="AROS-i386";;
	    aros-ppc)    fullsys="AROS-ppc";;
	    aros-x86_64) fullsys="AROS-x86_64";;
	esac
	mkdir -p "release/MCC_BetterString/Libs/MUI/$fullsys"
	cp -a mcc/bin_$os/BetterString.mcc "release/MCC_BetterString/Libs/MUI/$fullsys/"
	cp -a mcc/hotkeystring/bin_$os/HotkeyString.mcc "release/MCC_BetterString/Libs/MUI/$fullsys/"
	cp -a mcp/bin_$os/BetterString.mcp -a "release/MCC_BetterString/Libs/MUI/$fullsys/"
done

make -C mcp catalogs
for language in czech danish french german italian polish russian swedish; do
	mkdir -p "release/MCC_BetterString/Locale/Catalogs/$language"
	cp -a mcp/locale/$language.catalog "release/MCC_BetterString/Locale/Catalogs/$language/BetterString_mcp.catalog"
done

cp -a -R dist/* "release/"
cp -a AUTHORS ChangeLog COPYING "release/MCC_BetterString/"
cp -a doc/MCC_BetterString.readme "release/MCC_BetterString/ReadMe"
cp -a doc/MCC_BetterString.doc "release/MCC_BetterString/Developer/Autodocs/"
cp -a doc/MCC_HotkeyString.doc "release/MCC_BetterString/Developer/Autodocs/"
cp -a include/mui/BetterString_mcc.h "release/MCC_BetterString/Developer/C/include/mui/"
cp -a include/mui/HotkeyString_mcc.h "release/MCC_BetterString/Developer/C/include/mui/"
cp -a mcp/locale/BetterString_mcp.cd "release/MCC_BetterString/Locale/"

releasever=`grep "#define LIB_VERSION" mcc/version.h | awk '{ print $3 }'`
releaserev=`grep "#define LIB_REVISION" mcc/version.h | awk '{ print $3 }'`

echo "  MK MCC_BetterString-$releasever.$releaserev.lha"
find release -nowarn -name ".svn" -exec rm -rf {} \; 2>/dev/null
cd release
lha -ao5q ../MCC_BetterString-$releasever.$releaserev.lha *
