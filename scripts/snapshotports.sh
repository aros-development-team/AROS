#!/bin/bash

# Copyright © 2015, The AROS Development Team. All rights reserved.
# $Id$

# This script simplifies the task of creating snapshots of the ports.


set -e
set -u

date=`date +"%Y%m%d"`

root="$HOME/aros-snapshot-ports"
srcdir="$root/src"
arcdir="$root/archive/snapshots2/$date/Binaries"
portsdir="$HOME/aros-ports-src"
user="marust"

copy()
{
    local target="$1"   # e.g. "pc-i386"
    local reldir="$2"   # e.g. "Extras/Utilities/Text
    local pack="$3"     # e.g. "Annotate"
    local from="$root/build/$target/bin/$target/AROS/$reldir/$pack"
    local to="$root/bin/AROS-$date-$target-ports/$reldir"

    if [ -d "$from" ] ; then
        echo "Copying '$from' to '$to'"
        mkdir -p "$to"
        cp -r "$from" "$to"
        cp -r "$from.info" "$to"
    else
        echo "'$from' doesn't exist"
    fi
}

build()
{
    local target="$1"   # e.g. "pc-i386"
    local builddir="$root/build/$target"
    local arcname="AROS-$date-$target-ports"
    local bindir="$root/bin/$arcname"

    echo "Building $target in $builddir"
    rm "$builddir" -rf
    mkdir "$builddir" -p
    cd "$builddir"
    "$srcdir/configure" --target="$target" --with-portssources="$portsdir"
    make -s -j3
    make -s -j3 ports

    echo "Copying the binaries"
    mkdir "$bindir" -p

    copy "$target" "Extras/MultiMedia/Audio"        "Abcm2ps"
    copy "$target" "Extras/Development"             "BWBasic"
    copy "$target" "Extras/Development"             "CBMBasic"
    copy "$target" "Extras/Development"             "CFlow"
    copy "$target" "Extras/Development"             "Ctags"
    copy "$target" "Extras/Emu"                     "DOSBox"
    copy "$target" "Extras/Emu"                     "Scummvm"
    copy "$target" "Extras/Emu"                     "Vice"
    copy "$target" "Extras/Utilities/Filetool"      "Zaphod"
    copy "$target" "Extras/Games"                   "Abuse"
    copy "$target" "Extras/Games"                   "Adv770"
    copy "$target" "Extras/Games"                   "Biniax2"
    copy "$target" "Extras/Games"                   "Blobwars"
    copy "$target" "Extras/Games"                   "Bugsquish"
    copy "$target" "Extras/Games"                   "CircusLinux"
    copy "$target" "Extras/Games"                   "Defendguin"
    copy "$target" "Extras/Games"                   "GemDropX"
    copy "$target" "Extras/Games"                   "GnuRobbo"
    copy "$target" "Extras/Games"                   "Hurrican"
    copy "$target" "Extras/Games"                   "InterLogic"
    copy "$target" "Extras/Games"                   "KoboDeluxe"
    copy "$target" "Extras/Games"                   "Koules"
    copy "$target" "Extras/Games"                   "LMarbles"
    copy "$target" "Extras/Games"                   "LTris"
    copy "$target" "Extras/Games"                   "MadBomber"
    copy "$target" "Extras/Games"                   "Magnetic"
    copy "$target" "Extras/Games"                   "MultiPuzzle"
    copy "$target" "Extras/Games"                   "PenguinCommand"
    copy "$target" "Extras/Games"                   "Pushover"
    copy "$target" "Extras/Games"                   "SdlScavenger"
    copy "$target" "Extras/Games"                   "Vectoroids"
    copy "$target" "Extras/Games"                   "WormWars"
    copy "$target" "Extras/Games"                   "Xpired"
    copy "$target" "Extras/MultiMedia/Gfx"          "GrafX2"
    copy "$target" "Extras/MultiMedia/Gfx"          "Lunapaint"
    copy "$target" "Extras/MultiMedia/Gfx"          "Potrace"
    copy "$target" "Extras/MultiMedia/Gfx"          "ZuneView"
    copy "$target" "Extras/Office"                  "MUIbase"
    copy "$target" "Extras/Utilities/Print"         "A2ps"
    copy "$target" "Extras/Utilities/Scientific"    "Mathomatic"
    copy "$target" "Extras/Utilities/Scientific"    "MathX"
    copy "$target" "Extras/Utilities/Text"          "Annotate"
    copy "$target" "Extras/Utilities/Text"          "Antiword"
    copy "$target" "Extras/Utilities/Text"          "Gocr"
    copy "$target" "Extras/Utilities/Text"          "PolyglotMan"
    copy "$target" "Extras/MultiMedia/Video"        "ScreenRecorder"

    echo "Creating the archive $arcdir/$arcname.tar.bz2"
    mkdir -p "$arcdir"
    cd "$bindir"
    tar cfvj "$arcdir/$arcname.tar.bz2" .
    echo "Creating $arcdir/$arcname.tar.bz2.md5"
    cd $arcdir
    md5sum "$arcname.tar.bz2" > "$arcname.tar.bz2.md5"
}


if [ $# -eq 1 ] && [ "$1" == "-i" ] ; then
    rm "$root" -rf
    mkdir "$root"
    mkdir "$srcdir"
    svn checkout https://svn.aros.org/svn/aros/trunk/AROS "$srcdir"
    svn checkout https://svn.aros.org/svn/aros/trunk/contrib "$srcdir/contrib"
    svn checkout https://svn.aros.org/svn/aros/trunk/ports "$srcdir/ports"
    exit 0
fi

if [ $# -eq 1 ] && [ "$1" == "-b" ] ; then
    echo "Updating the sources"
    svn up "$srcdir" "$srcdir/contrib" "$srcdir/ports"

    # for some reason I can't build the pc variant ATM -- mazze
    #build pc-i386
    #build pc-i386
    build linux-i386
    build linux-x86_64
    build amiga-m68k
    build raspi-armhf
    exit 0
fi

if [ $# -eq 1 ] && [ "$1" == "-u" ] ; then
    echo "Uploading archives"
    scp -r "$root/archive/snapshots2" "$user@frs.sourceforge.net:userweb/htdocs/uploads"
    exit 1
fi

echo "Usage:"
echo "snapshotports -i  : create directories and do checkout of source"
echo "snapshotports -b  : build AROS and create the archives"
echo "snapshotports -u  : upload of archives to sourceforge"

exit 0
