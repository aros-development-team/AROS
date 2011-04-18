#!/bin/sh
#/***************************************************************************
#
# openurl.library - universal URL display and browser launcher library
# Copyright (C) 1998-2005 by Troels Walsted Hansen, et al.
# Copyright (C) 2005-2009 by openurl.library Open Source Team
#
# This library is free software; it has been placed in the public domain
# and you can freely redistribute it and/or modify it. Please note, however,
# that some components may be under the LGPL or GPL license.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# openurl.library project: http://sourceforge.net/projects/openurllib/
#
# $Id: AUTHORS 102 2009-05-27 22:22:46Z marust $
#
#***************************************************************************/

# openurl.library release build script
# invoke this script as "./mkrelease.sh" to build the release archives

rm -rf "release"
mkdir -p "release"
mkdir -p "release/OpenURL"
mkdir -p "release/OpenURL/C"
mkdir -p "release/OpenURL/Libs"
mkdir -p "release/OpenURL/Prefs"
mkdir -p "release/OpenURL/Catalogs"
mkdir -p "release/OpenURL/Developer"
mkdir -p "release/OpenURL/Developer/Autodocs"
mkdir -p "release/OpenURL/Developer/C"
mkdir -p "release/OpenURL/Developer/C/include"
mkdir -p "release/OpenURL/Developer/fd"
mkdir -p "release/OpenURL/Developer/sfd"
mkdir -p "release/OpenURL/Developer/xml"

for os in os3 os4 mos aros-i386 aros-ppc aros-x86_64; do

  make OS=$os clean
  make OS=$os DEBUG=

	case $os in
	    os3)         fullsys="AmigaOS3";;
	    os4)         fullsys="AmigaOS4";;
	    mos)         fullsys="MorphOS";;
	    aros-i386)   fullsys="AROS-i386";;
	    aros-ppc)    fullsys="AROS-ppc";;
	    aros-x86_64) fullsys="AROS-x86_64";;
	esac
	mkdir -p "release/OpenURL/C/$fullsys"
	mkdir -p "release/OpenURL/Libs/$fullsys"
	mkdir -p "release/OpenURL/Prefs/$fullsys"
  cp -a cmd/bin_$os/OpenURL "release/OpenURL/C/$fullsys/"
  cp -a library/bin_$os/openurl.library "release/OpenURL/Libs/$fullsys/"
  cp -a prefs/bin_$os/OpenURL "release/OpenURL/Prefs/$fullsys/"
  cp -a dist/OpenURL/Prefs/OpenURL.info "release/OpenURL/Prefs/$fullsys/"
	mkdir -p "release/OpenURL/Developer"
	mkdir -p "release/OpenURL/Developer/Autodocs"
	mkdir -p "release/OpenURL/Developer/fd"
	mkdir -p "release/OpenURL/Developer/sfd"
	mkdir -p "release/OpenURL/Developer/xml"
done

cp -a -R dist/* "release/"
rm -f "release/OpenURL/Prefs/OpenURL.info"
cp -a AUTHORS ChangeLog COPYING "release/OpenURL/"
cp -a developer/Autodocs/* "release/OpenURL/Developer/Autodocs/"
cp -a developer/fd/* "release/OpenURL/Developer/fd/"
cp -a -R developer/C/include/* "release/OpenURL/Developer/C/include/"
cp -a developer/sfd/* "release/OpenURL/Developer/sfd/"
cp -a developer/xml/* "release/OpenURL/Developer/xml/"

cp -a locale/OpenURL.cd "release/OpenURL/Catalogs/"
rm -f locale/*.catalog
make -C prefs catalogs
for language in french german italian polish swedish; do
  mkdir -p "release/OpenURL/Catalogs/$language"
  cp locale/$language.catalog "release/OpenURL/Catalogs/$language/OpenURL.catalog"
done

releasever=`grep "#define LIB_VERSION" library/version.h | awk '{ print $3 }'`
releaserev=`grep "#define LIB_REVISION" library/version.h | awk '{ print $3 }'`

echo "  MK OpenURL-$releasever.$releaserev.lha"
find release -nowarn -name ".svn" -exec rm -rf {} \; 2>/dev/null
cd release
rm -f ../OpenURL-$releasever.$releaserev.lha
lha -ao5q ../OpenURL-$releasever.$releaserev.lha *
