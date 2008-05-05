#!/bin/bash

BUILDBASEDIR=`pwd`

BINUTILS_VER=2.18.50
GCC_VER=4.3.0
NEWLIB_VER=1.16.0
GMP_VER=4.2.2
MPFR_VER=2.3.1

GDB_VER=6.7.1

WITHNEWLIB=true

BUILDTARGET=i386-elf
BUILDHOST=linux-x86_64

PORTSSOURCEDIR=$BUILDBASEDIR/bin/Source
BUILDHOSTOUTPUTDIR=$BUILDBASEDIR/bin/$BUILDHOST
BUILDHOSTPORTSDIR=$BUILDHOSTOUTPUTDIR/Ports
BUILDHOSTTOOLSDIR=$BUILDHOSTOUTPUTDIR/tools
BUILDHOSTGENDIR=$BUILDHOSTOUTPUTDIR/gen/tools
BUILDLOGFILETMP=$BUILDHOSTGENDIR/$BUILDTARGET/tmp.log

mkdir -p $BUILDHOSTTOOLSDIR
mkdir -p $BUILDHOSTGENDIR/$BUILDTARGET

echo "Building $BUILDTARGET cross compile tools for $BUILDHOST"

echo " - Downloading necessary packages"
mkdir -p $PORTSSOURCEDIR
cd $PORTSSOURCEDIR
if [ ! -e "$PORTSSOURCEDIR/gmp-$GMP_VER.tar.bz2" ]
    then mkdir -p $BUILDHOSTPORTSDIR/gmp
        wget http://ftp.sunet.se/pub/gnu/gmp/gmp-$GMP_VER.tar.bz2
fi
cd $PORTSSOURCEDIR
if [ ! -e "$PORTSSOURCEDIR/mpfr-$MPFR_VER.tar.bz2" ]
    then mkdir -p $BUILDHOSTPORTSDIR/mpfr
        wget http://www.mpfr.org/mpfr-current/mpfr-$MPFR_VER.tar.bz2
fi
cd $PORTSSOURCEDIR
if [ ! -e "$PORTSSOURCEDIR/binutils-$BINUTILS_VER.tar.bz2" ] 
    then mkdir -p $BUILDHOSTPORTSDIR/binutils
        wget ftp://sourceware.org/pub/binutils/snapshots/binutils-$BINUTILS_VER.tar.bz2
fi
if [ "$WITHNEWLIB" == "true" ]
    then cd $PORTSSOURCEDIR
    if [ ! -e "$PORTSSOURCEDIR/newlib-$NEWLIB_VER.tar.gz" ]
        then mkdir -p $BUILDHOSTPORTSDIR/newlib
            wget ftp://sources.redhat.com/pub/newlib/newlib-$NEWLIB_VER.tar.gz
    fi
fi
cd $PORTSSOURCEDIR
if [ ! -e "$PORTSSOURCEDIR/gcc-$GCC_VER.tar.bz2" ] 
    then mkdir -p $BUILDHOSTPORTSDIR/gcc
        wget ftp://ftp.sunet.se/pub/gnu/gcc/releases/gcc-$GCC_VER/gcc-$GCC_VER.tar.bz2
fi

echo " - Extracting packages"
if [ ! -e "$BUILDHOSTPORTSDIR/gmp/gmp-$GMP_VER" ]
    then echo "       >> gmp-$GMP_VER.tar.bz2"
        cd "$BUILDHOSTPORTSDIR/gmp"
        tar -xf $PORTSSOURCEDIR/gmp-$GMP_VER.tar.bz2
fi
if [ ! -e "$BUILDHOSTPORTSDIR/mpfr/mpfr-$MPFR_VER" ]
    then echo "       >> mpfr-$MPFR_VER.tar.bz2"
        cd "$BUILDHOSTPORTSDIR/mpfr"
        tar -xf $PORTSSOURCEDIR/mpfr-$MPFR_VER.tar.bz2
fi
if [ ! -e "$BUILDHOSTPORTSDIR/binutils/binutils-$BINUTILS_VER" ]
    then echo "       >> binutils-$BINUTILS_VER.tar.bz2"
        cd "$BUILDHOSTPORTSDIR/binutils"
        tar -xf $PORTSSOURCEDIR/binutils-$BINUTILS_VER.tar.bz2
fi
if [ ! -e "$BUILDHOSTPORTSDIR/newlib/newlib-$NEWLIB_VER" ]
    then echo "       >> newlib-$NEWLIB_VER.tar.gz"
        cd "$BUILDHOSTPORTSDIR/newlib"
        tar -xf $PORTSSOURCEDIR/newlib-$NEWLIB_VER.tar.gz
fi
if [ ! -e "$BUILDHOSTPORTSDIR/gcc/gcc-$GCC_VER" ]
    then echo "       >> gcc-$GCC_VER.tar.bz2"
        cd "$BUILDHOSTPORTSDIR/gcc"
        tar -xf $PORTSSOURCEDIR/gcc-$GCC_VER.tar.bz2
fi

export target=$BUILDTARGET

if [ -e "$BUILDHOSTPORTSDIR/gmp/gmp-$GMP_VER" ]
    then echo " - Building GMP $GMP_VER (prerequisite)"
        mkdir -p $BUILDHOSTGENDIR/gmp-$GMP_VER
        cd $BUILDHOSTGENDIR/gmp-$GMP_VER
        if [ ! -e "$BUILDHOSTGENDIR/gmp-$GMP_VER/.configured" ]
            then echo "       Configuring .."
                rm $BUILDHOSTGENDIR/gmp-config.log 2>/dev/null >/dev/null
                bash $BUILDHOSTPORTSDIR/gmp/gmp-$GMP_VER/configure --prefix=$BUILDHOSTOUTPUTDIR 2>$BUILDLOGFILETMP >$BUILDLOGFILETMP
                cat $BUILDLOGFILETMP >> $BUILDHOSTGENDIR/gmp-config.log
                echo "done" > $BUILDHOSTGENDIR/gmp-$GMP_VER/.configured
        fi
        echo "       Compiling .."
        rm $BUILDHOSTGENDIR/gmp-build.log 2>/dev/null >/dev/null
        make 2>$BUILDLOGFILETMP >$BUILDLOGFILETMP
        cat $BUILDLOGFILETMP >> $BUILDHOSTGENDIR/gmp-build.log
        make check 2>$BUILDLOGFILETMP >$BUILDLOGFILETMP
        cat $BUILDLOGFILETMP >> $BUILDHOSTGENDIR/gmp-build.log
        make install 2>$BUILDLOGFILETMP >$BUILDLOGFILETMP
        cat $BUILDLOGFILETMP >> $BUILDHOSTGENDIR/gmp-build.log
        make clean 2>$BUILDLOGFILETMP >$BUILDLOGFILETMP
        cat $BUILDLOGFILETMP >> $BUILDHOSTGENDIR/gmp-build.log
fi

if [ -e "$BUILDHOSTPORTSDIR/mpfr/mpfr-$MPFR_VER" ]
    then echo " - Building MPFR $MPFR_VER (prerequisite)"
        mkdir -p $BUILDHOSTGENDIR/mpfr-$MPFR_VER
        cd $BUILDHOSTGENDIR/mpfr-$MPFR_VER
        if [ ! -e "$BUILDHOSTGENDIR/mpfr-$MPFR_VER/.configured" ]
            then echo "       Configuring .."
                rm $BUILDHOSTGENDIR/mpfr-config.log 2>/dev/null >/dev/null
                $BUILDHOSTPORTSDIR/mpfr/mpfr-$MPFR_VER/configure --prefix=$BUILDHOSTOUTPUTDIR 2>$BUILDLOGFILETMP >$BUILDLOGFILETMP
                cat $BUILDLOGFILETMP >> $BUILDHOSTGENDIR/mpfr-config.log
                echo "done" > $BUILDHOSTGENDIR/mpfr-$MPFR_VER/.configured
        fi
        echo "       Compiling .."
        rm $BUILDHOSTGENDIR/mpfr-build.log 2>/dev/null >/dev/null
        make 2>$BUILDLOGFILETMP >$BUILDLOGFILETMP
        cat $BUILDLOGFILETMP >> $BUILDHOSTGENDIR/mpfr-build.log
        make check 2>$BUILDLOGFILETMP >$BUILDLOGFILETMP
        cat $BUILDLOGFILETMP >> $BUILDHOSTGENDIR/mpfr-build.log
        make install 2>$BUILDLOGFILETMP >$BUILDLOGFILETMP
        cat $BUILDLOGFILETMP >> $BUILDHOSTGENDIR/mpfr-build.log
        make clean 2>$BUILDLOGFILETMP >$BUILDLOGFILETMP
        cat $BUILDLOGFILETMP >> $BUILDHOSTGENDIR/mpfr-build.log
fi

if [ -e "$BUILDHOSTPORTSDIR/binutils/binutils-$BINUTILS_VER" ]
    then echo " - Building toolchain"
        mkdir -p $BUILDHOSTGENDIR/$BUILDTARGET/binutils-$BINUTILS_VER
        cd $BUILDHOSTGENDIR/$BUILDTARGET/binutils-$BINUTILS_VER
        if [ ! -e "$BUILDHOSTGENDIR/$BUILDTARGET/binutils-$BINUTILS_VER/.configured" ]
            then echo "       Configuring .."
                rm $BUILDHOSTGENDIR/$BUILDTARGET/binutils-config.log 2>/dev/null >/dev/null
                $BUILDHOSTPORTSDIR/binutils/binutils-$BINUTILS_VER/configure --target=$target --prefix=$BUILDHOSTTOOLSDIR --bindir=$BUILDHOSTTOOLSDIR --disable-nls --disable-shared --disable-threads 2>$BUILDLOGFILETMP >$BUILDLOGFILETMP
                cat $BUILDLOGFILETMP >> $BUILDHOSTGENDIR/$BUILDTARGET/binutils-config.log
                echo "done" > $BUILDHOSTGENDIR/$BUILDTARGET/binutils-$BINUTILS_VER/.configured
        fi
        echo "       Compiling .."
        rm $BUILDHOSTGENDIR/$BUILDTARGET/binutils-build.log 2>/dev/null >/dev/null
        make 2>$BUILDLOGFILETMP >$BUILDLOGFILETMP
        cat $BUILDLOGFILETMP >> $BUILDHOSTGENDIR/$BUILDTARGET/binutils-build.log
        make install 2>$BUILDLOGFILETMP >$BUILDLOGFILETMP
        cat $BUILDLOGFILETMP >> $BUILDHOSTGENDIR/$BUILDTARGET/binutils-build.log
        make clean 2>$BUILDLOGFILETMP >$BUILDLOGFILETMP
        cat $BUILDLOGFILETMP >> $BUILDHOSTGENDIR/$BUILDTARGET/binutils-build.log
fi

if [ -e "$BUILDHOSTPORTSDIR/gcc/gcc-$GCC_VER" ]
    then echo " - Building GCC $GCC_VER"
        if [ -e "$BUILDHOSTPORTSDIR/newlib/newlib-$NEWLIB_VER" ]
            then echo "       Creating prerequisite library aliases"
                cd $BUILDHOSTPORTSDIR/gcc/gcc-$GCC_VER
                ln -s $BUILDHOSTPORTSDIR/newlib/newlib-$NEWLIB_VER/newlib . 2>/dev/null >/dev/null
                ln -s $BUILDHOSTPORTSDIR/newlib/newlib-$NEWLIB_VER/libgloss . 2>/dev/null >/dev/null
                export NEWLIB_FLAGS="--with-newlib"
        fi
        mkdir -p $BUILDHOSTGENDIR/$BUILDTARGET/gcc-$GCC_VER
        cd $BUILDHOSTGENDIR/$BUILDTARGET/gcc-$GCC_VER
        if [ ! -e "$BUILDHOSTGENDIR/$BUILDTARGET/gcc-$GCC_VER/.configured" ]
            then echo "       Configuring .."
                rm $BUILDHOSTGENDIR/$BUILDTARGET/gcc-config.log 2>/dev/null >/dev/null
                $BUILDHOSTPORTSDIR/gcc/gcc-$GCC_VER/configure --target=$target --prefix=$BUILDHOSTTOOLSDIR --bindir=$BUILDHOSTTOOLSDIR --enable-languages=c,c++ --disable-libssp --disable-nls --disable-shared --disable-threads --disable-multilib --with-as=$BUILDHOSTTOOLSDIR/$target-as --with-gnu-as --with-ld=$BUILDHOSTTOOLSDIR/$target-ld --with-gnu-ld --with-gmp=$BUILDHOSTOUTPUTDIR --with-mpfr=$BUILDHOSTOUTPUTDIR  $NEWLIB_FLAGS 2>$BUILDLOGFILETMP >$BUILDLOGFILETMP
                cat $BUILDLOGFILETMP >> $BUILDHOSTGENDIR/$BUILDTARGET/gcc-config.log
                echo "done" > $BUILDHOSTGENDIR/$BUILDTARGET/gcc-$GCC_VER/.configured
        fi
        echo "       Compiling .."
        rm $BUILDHOSTGENDIR/$BUILDTARGET/gcc-build.log 2>/dev/null >/dev/null
        make "CFLAGS = -DREENTRANT_SYSCALLS_PROVIDED" 2>$BUILDLOGFILETMP >$BUILDLOGFILETMP
        cat $BUILDLOGFILETMP >> $BUILDHOSTGENDIR/$BUILDTARGET/gcc-build.log
        make install 2>$BUILDLOGFILETMP >$BUILDLOGFILETMP
        cat $BUILDLOGFILETMP >> $BUILDHOSTGENDIR/$BUILDTARGET/gcc-build.log
fi

rm $BUILDLOGFILETMP 2>/dev/null >/dev/null

echo "Build of $BUILDTARGET cross compile tools completed"
