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
        if [ -f "$from.info" ] ; then
            cp "$from.info" "$to"
        fi
    else
        echo "'$from' doesn't exist"
    fi
}

copyenv()
{
    local target="$1"   # e.g. "pc-i386"
    local reldir="Prefs/Env-Archive/SYS/Packages"
    local variable="$2" # e.g. "Lua"
    local from="$root/build/$target/bin/$target/AROS/$reldir/$variable"
    local to="$root/bin/AROS-$date-$target-ports/$reldir"

    if [ -f "$from" ] ; then
        echo "Copying '$from' to '$to'"
        mkdir -p "$to"
        cp "$from" "$to"
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

    #echo "Building $target in $builddir"
    #rm "$builddir" -rf
    #mkdir "$builddir" -p
    #cd "$builddir"
    #"$srcdir/configure" --target="$target" --with-portssources="$portsdir" --with-binutils-version=2.25 --with-gcc-version=6.3.0
    #make -s -j4
    #make -s -j4 ports

    echo "Copying the binaries"
    mkdir "$bindir" -p

    copy "$target" "Extras/Developer"               "BWBasic"
    copy "$target" "Extras/Developer"               "CBMBasic"
    copy "$target" "Extras/Developer"               "CFlow"
    copy "$target" "Extras/Developer"               "Ctags"
    copy "$target" "Extras/Emu"                     "DOSBox"
    copy "$target" "Extras/Emu"                     "Scummvm"
    copy "$target" "Extras/Emu"                     "Vbam"
    copy "$target" "Extras/Emu"                     "Vice"
    copy "$target" "Extras/Games/Platform"          "Abuse"
    copy "$target" "Extras/Games/Action"            "Biniax2"
    copy "$target" "Extras/Games/Platform"          "Blobwars"
    copy "$target" "Extras/Games/Action"            "Bugsquish"
    copy "$target" "Extras/Games/Action"            "CircusLinux"
    copy "$target" "Extras/Games/Action"            "Defendguin"
    copy "$target" "Extras/Games/Puzzle"            "GemDropX"
    copy "$target" "Extras/Games/Action"            "GnuJump"
    copy "$target" "Extras/Games/Action"            "GnuRobbo"
    copy "$target" "Extras/Games/Platform"          "Hurrican"
    copy "$target" "Extras/Games/Puzzle"            "InterLogic"
    copy "$target" "Extras/Games/Action"            "KoboDeluxe"
    copy "$target" "Extras/Games/Action"            "Koules"
    copy "$target" "Extras/Games/Puzzle"            "LMarbles"
    copy "$target" "Extras/Games/Puzzle"            "LTris"
    copy "$target" "Extras/Games/Action"            "MadBomber"
    copy "$target" "Extras/Games/Adventure"         "Magnetic"
    copy "$target" "Extras/Games/Puzzle"            "MultiPuzzle"
    copy "$target" "Extras/Games/Action"            "PenguinCommand"
    copy "$target" "Extras/Games/Puzzle"            "Pushover"
    copy "$target" "Extras/Games/Action"            "Rocksndiamonds"
    copy "$target" "Extras/Games/Action"            "SdlScavenger"
    copy "$target" "Extras/Games/Action"            "Vectoroids"
    copy "$target" "Extras/Games/Action"            "WormWars"
    copy "$target" "Extras/Games/Action"            "Xpired"
    copy "$target" "Extras/MultiMedia/Audio"        "Abcm2ps"
    copy "$target" "Extras/MultiMedia/Audio"        "MilkyTracker"
    copy "$target" "Extras/MultiMedia/Audio"        "Timidity"
    copy "$target" "Extras/MultiMedia/Gfx"          "GrafX2"
    copy "$target" "Extras/MultiMedia/Gfx"          "Lodepaint"
    copy "$target" "Extras/MultiMedia/Gfx"          "Potrace"
    copy "$target" "Extras/MultiMedia/Gfx"          "ZunePaint"
    copy "$target" "Extras/MultiMedia/Gfx"          "ZuneView"
    copy "$target" "Extras/MultiMedia/Video"        "ScreenRecorder"
    copy "$target" "Extras/Office"                  "MUIbase"
    copy "$target" "Extras/Utilities/Archive"       "ZuneARC"
    copy "$target" "Extras/Utilities/Filetool"      "Zaphod"
    copy "$target" "Extras/Utilities/Print"         "A2ps"
    copy "$target" "Extras/Utilities/Scientific"    "Mathomatic"
    copy "$target" "Extras/Utilities/Scientific"    "MathX"
    copy "$target" "Extras/Utilities/Text"          "Annotate"
    copy "$target" "Extras/Utilities/Text"          "Antiword"
    copy "$target" "Extras/Utilities/Text"          "Gocr"
    copy "$target" "Extras/Utilities/Text"          "PolyglotMan"
    copy "$target" "Extras/Utilities/Text"          "Vim"

    # copy package variables
    copyenv "$target" "Freepats"
    copyenv "$target" "MUIbase"
    copyenv "$target" "Vim"

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
    echo "Update the sources"
    svn up "$srcdir" "$srcdir/contrib" "$srcdir/ports"

    build pc-i386
    build pc-x86_64
    build amiga-m68k
    #build raspi-armhf
    exit 0
fi

if [ $# -eq 1 ] && [ "$1" == "-u" ] ; then
    echo "Uploading archives"
    scp -r "$root/archive/snapshots2" "$user,aros@web.sourceforge.net:/home/project-web/aros/uploads"
    exit 0
fi

echo "Usage:"
echo "snapshotports -i  : create directories and do checkout of source"
echo "snapshotports -b  : build AROS and create the archives"
echo "snapshotports -u  : upload of archives to sourceforge"

exit 0
