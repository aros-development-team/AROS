#!/bin/sh

############################################################################
#
# NList MUI custom classes
#
# Copyright (C) 1996-2001 by Gilles Masson (NList.mcc)
# Copyright (C) 1999-2001 by Carsten Scholling (NListtree.mcc)
# Copyright (C) 2006      by Daniel Allsopp (NBitmap.mcc)
# Copyright (C) 2001-2014 NList Open Source Team
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
# NList classes Support Site:  http://www.sf.net/projects/nlist-classes
#
# $Id: mkrelease.sh 513 2010-09-10 06:47:11Z thboeckel $
#
############################################################################

# NList classes release build script
# invoke this script as "./mkrelease.sh <revision>" to build the release archives

if [ "$1" = "" ]; then
	echo "no release revision specified"
	exit
fi

rm -rf "release"
mkdir -p "release"
mkdir -p "release/MCC_NList"
mkdir -p "release/MCC_NList/Demos"
mkdir -p "release/MCC_NList/Developer"
mkdir -p "release/MCC_NList/Developer/Autodocs"
mkdir -p "release/MCC_NList/Developer/Autodocs/MCC"
mkdir -p "release/MCC_NList/Developer/C"
mkdir -p "release/MCC_NList/Developer/C/Examples"
mkdir -p "release/MCC_NList/Developer/C/include"
mkdir -p "release/MCC_NList/Developer/C/include/mui"
mkdir -p "release/MCC_NList/Libs"
mkdir -p "release/MCC_NList/Libs/MUI"
mkdir -p "release/MCC_NList/Locale"
mkdir -p "release/MCC_NList/Locale/Catalogs"

make -C nbalance_mcc release
make -C nbitmap_mcc release
make -C nfloattext_mcc release
make -C nlist_mcc release
make -C nlisttree_mcc release
make -C nlistview_mcc release
make -C nlisttree_mcp release
make -C nlistviews_mcp release
make -C demo release

for os in os3 os4 mos aros-i386 aros-ppc aros-x86_64; do
	case $os in
	    os3)         fullsys="AmigaOS3";;
	    os4)         fullsys="AmigaOS4";;
	    mos)         fullsys="MorphOS";;
	    aros-i386)   fullsys="AROS-i386";;
	    aros-ppc)    fullsys="AROS-ppc";;
	    aros-x86_64) fullsys="AROS-x86_64";;
	esac
	mkdir -p "release/MCC_NList/Demos/$fullsys"
	mkdir -p "release/MCC_NList/Libs/MUI/$fullsys"
	cp -a demo/bin_$os/NBitmap-Demo "release/MCC_NList/Demos/$fullsys/"
	cp -a nbitmap_mcc/icon.png "release/MCC_NList/Demos/$fullsys/"
	cp -a demo/bin_$os/NList-Demo "release/MCC_NList/Demos/$fullsys/"
	cp -a demo/bin_$os/NListtree-Demo "release/MCC_NList/Demos/$fullsys/"
	cp -a nbalance_mcc/bin_$os/NBalance.mcc "release/MCC_NList/Libs/MUI/$fullsys/"
	cp -a nbitmap_mcc/bin_$os/NBitmap.mcc "release/MCC_NList/Libs/MUI/$fullsys/"
	cp -a nfloattext_mcc/bin_$os/NFloattext.mcc "release/MCC_NList/Libs/MUI/$fullsys/"
	cp -a nlist_mcc/bin_$os/NList.mcc "release/MCC_NList/Libs/MUI/$fullsys/"
	cp -a nlisttree_mcc/bin_$os/NListtree.mcc "release/MCC_NList/Libs/MUI/$fullsys/"
	cp -a nlistview_mcc/bin_$os/NListview.mcc "release/MCC_NList/Libs/MUI/$fullsys/"
	cp -a nlisttree_mcp/bin_$os/NListtree.mcp -a "release/MCC_NList/Libs/MUI/$fullsys/"
	cp -a nlistviews_mcp/bin_$os/NListviews.mcp -a "release/MCC_NList/Libs/MUI/$fullsys/"
done

make -C nlisttree_mcp catalogs
for language in `ls nlisttree_mcp/locale/*.catalog`; do
  catalog=$(basename "$language")
  lang="${catalog%.*}"
  mkdir -p "release/MCC_NList/Locale/Catalogs/${lang}"
  cp -a ${language} "release/MCC_NList/Locale/Catalogs/${lang}/NListtree_mcp.catalog"
done
make -C nlistviews_mcp catalogs
for language in `ls nlistviews_mcp/locale/*.catalog`; do
  catalog=$(basename "$language")
  lang="${catalog%.*}"
  mkdir -p "release/MCC_NList/Locale/Catalogs/${lang}"
  cp -a ${language} "release/MCC_NList/Locale/Catalogs/${lang}/NListviews_mcp.catalog"
done

cp -a -R dist/* "release/"
cp -a AUTHORS ChangeLog COPYING "release/MCC_NList/"
cp -a docs/ReadMe "release/MCC_NList/ReadMe"
cp -a docs/*.doc "release/MCC_NList/Developer/Autodocs/MCC/"
cp -a demo/*.c "release/MCC_NList/Developer/C/Examples/"
cp -a demo/Makefile "release/MCC_NList/Developer/C/Examples/"
cp -a include/mui/NBalance_mcc.h "release/MCC_NList/Developer/C/include/mui/"
cp -a include/mui/NBitmap_mcc.h "release/MCC_NList/Developer/C/include/mui/"
cp -a include/mui/NFloattext_mcc.h "release/MCC_NList/Developer/C/include/mui/"
cp -a include/mui/NList_mcc.h "release/MCC_NList/Developer/C/include/mui/"
cp -a include/mui/NListtree_mcc.h "release/MCC_NList/Developer/C/include/mui/"
cp -a include/mui/NListview_mcc.h "release/MCC_NList/Developer/C/include/mui/"
cp -a nlisttree_mcp/locale/NListtree_mcp.pot "release/MCC_NList/Locale/"
cp -a nlistviews_mcp/locale/NListviews_mcp.pot "release/MCC_NList/Locale/"

echo "  MK MCC_NList-0.$1.lha"
find release -nowarn -name ".svn" -exec rm -rf {} \; 2>/dev/null
cd release
lha -ao5q ../MCC_NList-0.$1.lha *
cp MCC_NList/ReadMe ../MCC_NList-0.$1.readme
cd ..
