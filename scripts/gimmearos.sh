#!/bin/bash

# Script for building AROS from the Subversion repository

# Currently only a limited amount of Linux distros is supported.
# If you improve this script, send modifications back to me, please.
# Matthias Rustler, mailto:mrustler@gmx.de

# This script is public domain. Use it at your own risk.

# $VER: gimmearos.sh 1.10 (28.07.2013)

curdir="`pwd`"
srcdir="aros-src"
srcdir_v0="aros-src-v0"
portsdir="ports-src"
cpucoresforcompile="2"

install_pkg()
{
    if [ $# -ne 2 ]
    then
        echo "install_pkg() needs 2 arguments"
        exit 100
    fi
    echo -e "\nInstalling " $2
    sudo $1 $2
    if [ $? -ne 0 ]
    then
        echo -e "\n    install failed. Script cancelled."
        exit 100
    fi
}



echo -e "\n\nScript for downloading and building of AROS"
echo -e "============================================"
echo -e "\nStep 1: install prerequisites"
echo -e "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
echo -e "The build system needs some packages to do its job."
echo -e "If you are asked for a password enter you admin password."
echo -e "\n1 .. Get packages with apt-get for Debian and similar (e.g. Ubuntu)"
echo -e "     for building 32-bit AROS on 32-bit Linux or 64-bit-AROS on 64-bit-Linux"
echo -e "2 .. As 1) but with additional packages for building 32-bit AROS"
echo -e "     on 64-bit Linux"
echo -e "3 .. Get packages with yum for Fedora"
echo -e "4 .. Get packages with pacman for Arch"
echo -e "9 .. Skip this step"
echo -e "0 .. Exit"

echo -e "\nEnter number and press <Enter>:"

read input
case "$input" in
    1 ) echo -e "\nInstalling prerequisites with apt-get..."
        install_pkg "apt-get install" subversion
        install_pkg "apt-get install" git-core
        install_pkg "apt-get install" gcc
        install_pkg "apt-get install" make
        install_pkg "apt-get install" gawk
        install_pkg "apt-get install" bison
        install_pkg "apt-get install" flex
        install_pkg "apt-get install" bzip2
        install_pkg "apt-get install" netpbm
        install_pkg "apt-get install" autoconf
        install_pkg "apt-get install" automake
        install_pkg "apt-get install" libx11-dev
        install_pkg "apt-get install" libxext-dev
        install_pkg "apt-get install" libc6-dev
        install_pkg "apt-get install" liblzo2-dev
        install_pkg "apt-get install" libxxf86vm-dev
        install_pkg "apt-get install" libpng12-dev
        install_pkg "apt-get install" gcc-multilib
        install_pkg "apt-get install" libsdl1.2-dev
        install_pkg "apt-get install" byacc
        ;;

    2 ) echo -e "\nInstalling prerequisites with apt-get..."
        install_pkg "apt-get install" subversion
        install_pkg "apt-get install" git-core
        install_pkg "apt-get install" gcc
        install_pkg "apt-get install" make
        install_pkg "apt-get install" gawk
        install_pkg "apt-get install" bison
        install_pkg "apt-get install" flex
        install_pkg "apt-get install" bzip2
        install_pkg "apt-get install" netpbm
        install_pkg "apt-get install" autoconf
        install_pkg "apt-get install" automake
        install_pkg "apt-get install" libx11-dev
        install_pkg "apt-get install" libxext-dev
        install_pkg "apt-get install" libc6-dev
        install_pkg "apt-get install" liblzo2-dev
        install_pkg "apt-get install" libxxf86vm-dev
        install_pkg "apt-get install" libpng12-dev
        install_pkg "apt-get install" gcc-multilib

        install_pkg "apt-get install" libc6-dev-i386
        install_pkg "apt-get install" lib32gcc1
        install_pkg "apt-get install" ia32-libs
        install_pkg "apt-get install" libsdl1.2-dev
        install_pkg "apt-get install" byacc
        ;;

    3 ) echo -e "\nInstalling prerequisites with yum..."
        install_pkg "yum install" subversion
        install_pkg "yum install" git-core
        install_pkg "yum install" gcc
        install_pkg "yum install" gawk
        install_pkg "yum install" bison
        install_pkg "yum install" flex
        install_pkg "yum install" bzip2
        install_pkg "yum install" netpbm
        install_pkg "yum install" autoconf
        install_pkg "yum install" automake
        install_pkg "yum install" libX11-devel
        install_pkg "yum install" glibc-devel
        install_pkg "yum install" lzo-devel
        ;;

    4 ) echo -e "\nInstalling prerequisites with pacman.."
        echo -e "\nUpdating the List of software"
        echo -e "\nEnter sudo password"
        sudo pacman -Sy
        install_pkg "pacman --needed --noconfirm -S" subversion
        install_pkg "pacman --needed --noconfirm -S" git-core
        install_pkg "pacman --needed --noconfirm -S" gcc
        install_pkg "pacman --needed --noconfirm -S" gawk
        install_pkg "pacman --needed --noconfirm -S" bison
        install_pkg "pacman --needed --noconfirm -S" flex
        install_pkg "pacman --needed --noconfirm -S" bzip2
        install_pkg "pacman --needed --noconfirm -S" netpbm
        install_pkg "pacman --needed --noconfirm -S" autoconf
        install_pkg "pacman --needed --noconfirm -S" automake
        #it appears as though the libx11-dev,libc6-dev,liblzo2-dev is not needed on arch
        ;;
         
    0 ) exit 0
        ;;
esac

cd "$curdir"

input=""
until [ "$input" = "9" ]
do
    echo -e "\nStep 2: Get the sources from the repository"
    echo -e "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
    echo -e "\nYou can either use Subversion or Git. Git doesn't require"
    echo -e   "a password, but you'll get only read-only access."
    echo -e   "The repositories will be checked out into the current directory."
    echo -e "\nABI V1 is the current development version which is no longer"
    echo -e   "binary compatible with V0."
    echo -e "\nABI V1 with | ABI V0 with | ABI V1   |"
    echo -e   "Subversion  | Subversion  | with GIT |       Content"
    echo -e   "------------+-------------+----------+--------------------------"
    echo -e   "     1      |      11     |    21    | Get AROS core (required)"
    echo -e   "     2      |      12     |    22    | Get contrib (optional)"
    echo -e "\n     3      |      13     |    23    | Get ports source (optional, needs contrib)"
    echo -e "\n     4      |      14     |    ---   | Get documentation source (optional)"
    echo -e   "     5      |      15     |    ---   | Get binaries (wallpapers, logos etc.) (optional)"
    echo -e "\n9 .. Leave loop, goto next step"
    echo -e   "0 .. Exit"

    echo -e "\nEnter number and press <Enter>:"

    read input
    case "$input" in
        1 ) echo -e "\nGetting AROS V1 core with Subversion...\n"
            svn checkout https://svn.aros.org/svn/aros/trunk/AROS "$srcdir"
            ;;
        2 ) echo -e "\nGetting contrib V1 with Subversion...\n"
            svn checkout https://svn.aros.org/svn/aros/trunk/contrib "$srcdir/contrib"
            ;;
        3 ) echo -e "\nGetting ports V1 with Subversion...\n"
            svn checkout https://svn.aros.org/svn/aros/trunk/ports "$srcdir/ports"
            ;;
        4 ) echo -e "\nGetting documentation V1 with Subversion...\n"
            svn checkout https://svn.aros.org/svn/aros/trunk/documentation "$srcdir/documentation"
            ;;
        5 ) echo -e "\nGetting binaries V1 with Subversion...\n"
            svn checkout https://svn.aros.org/svn/aros/trunk/binaries "$srcdir/binaries"
            ;;

        11 ) echo -e "\nGetting AROS V0 core with Subversion...\n"
            svn checkout https://svn.aros.org/svn/aros/branches/ABI_V0/AROS "$srcdir_v0"
            ;;
        12 ) echo -e "\nGetting contrib V0 with Subversion...\n"
            svn checkout https://svn.aros.org/svn/aros/branches/ABI_V0/contrib "$srcdir_v0/contrib"
            ;;
        13 ) echo -e "\nGetting ports V0 with Subversion...\n"
            svn checkout https://svn.aros.org/svn/aros/branches/ABI_V0/ports "$srcdir_v0/ports"
            ;;
        14 ) echo -e "\nGetting documentation V1 with Subversion...\n"
            svn checkout https://svn.aros.org/svn/aros/branches/ABI_V0/documentation "$srcdir_v0/documentation"
            ;;
        15 ) echo -e "\nGetting binaries V1 with Subversion...\n"
            svn checkout https://svn.aros.org/svn/aros/branches/ABI_V0/binaries "$srcdir_v0/binaries"
            ;;

        21 ) echo -e "\nGetting AROS V1 core with Git...\n"
            git clone git://repo.or.cz/AROS.git "$srcdir"
            ;;
        22 ) echo -e "\nGetting contrib V1 with Git...\n"
            git clone git://repo.or.cz/AROS-Contrib.git "$srcdir/contrib"
            ;;
        23 ) echo -e "\nGetting ports V1 with Git...\n"
            git clone git://repo.or.cz/AROS-Ports.git "$srcdir/ports"
            ;;

        0 ) exit 0
            ;;
    esac
done

cd "$curdir"


input=""
until [ "$input" = "9" ]
do
    cd "$curdir"

    echo -e "\nStep 3: Configuring"
    echo -e   "~~~~~~~~~~~~~~~~~~~"
    echo -e "\nABI V1 | ABI V0 |"
    echo -e   "-------+--------+----------------------------"
    echo -e   "  1    |   11   | linux-i386   (32-bit) debug"
    echo -e   "  2    |   12   | linux-i386   (32-bit)"
    echo -e   "  3    |   13   | linux-x86_64 (64-bit) debug"
    echo -e   "  4    |   14   | linux-x86_64 (64-bit)"
    echo -e   "  5    |   15   | pc-i386      (32-bit)"
    echo -e   "  6    |   16   | pc-x86_64    (64-bit)"
    echo -e "\n9 .. Leave loop, goto next step"
    echo -e   "0 .. Exit"

    echo -e "\nEnter number and press <Enter>:"

    read input
    case "$input" in
        1 ) echo -e "\nConfiguring linux-i386 V1 with full debug...\n"
            mkdir -p "$portsdir"
            mkdir -p aros-linux-i386-dbg
            cd aros-linux-i386-dbg
            "../$srcdir/configure" --target=linux-i386 --enable-debug=all --with-portssources="$curdir/$portsdir"
            ;;
        2 ) echo -e "\nConfiguring linux-i386 V1 without debug...\n"
            mkdir -p "$portsdir"
            mkdir -p aros-linux-i386
            cd aros-linux-i386
            "../$srcdir/configure" --target=linux-i386 --with-portssources="$curdir/$portsdir"
            ;;
        3 ) echo -e "\nConfiguring linux-x86_64 V1 with full debug...\n"
            mkdir -p "$portsdir"
            mkdir -p aros-linux-x86_64-dbg
            cd aros-linux-x86_64-dbg
            "../$srcdir/configure" --target=linux-x86_64 --enable-debug=all --with-portssources="$curdir/$portsdir"
            ;;
        4 ) echo -e "\nConfiguring linux-x86_64 V1 without debug...\n"
            mkdir -p "$portsdir"
            mkdir -p aros-linux-x86_64
            cd aros-linux-x86_64
            "../$srcdir/configure" --target=linux-x86_64 --with-portssources="$curdir/$portsdir"
            ;;
        5 ) echo -e "\nConfiguring pc-i386 V1...\n"
            mkdir -p "$portsdir"
            mkdir -p aros-pc-i386
            cd aros-pc-i386
            "../$srcdir/configure" --target=pc-i386 --with-portssources="$curdir/$portsdir"
            ;;
        6 ) echo -e "\nConfiguring pc-x86_64 V1...\n"
            mkdir -p "$portsdir"
            mkdir -p aros-pc-x86_64
            cd aros-pc-x86_64
            "../$srcdir/configure" --target=pc-x86_64 --with-portssources="$curdir/$portsdir"
            ;;


        11 ) echo -e "\nConfiguring linux-i386 V0 with full debug...\n"
            mkdir -p "$portsdir"
            mkdir -p aros-linux-i386-v0-dbg
            cd aros-linux-i386-v0-dbg
            "../$srcdir_v0/configure" --target=linux-i386 --enable-debug=all --with-portssources="$curdir/$portsdir"
            ;;
        12 ) echo -e "\nConfiguring linux-i386 V0 without debug...\n"
            mkdir -p "$portsdir"
            mkdir -p aros-linux-i386-v0
            cd aros-linux-i386-v0
            "../$srcdir_v0/configure" --target=linux-i386 --with-portssources="$curdir/$portsdir"
            ;;
        13 ) echo -e "\nConfiguring linux-x86_64 V0 with full debug...\n"
            mkdir -p "$portsdir"
            mkdir -p aros-linux-x86_64-v0-dbg
            cd aros-linux-x86_64-v0-dbg
            "../$srcdir_v0/configure" --target=linux-x86_64 --enable-debug=all --with-portssources="$curdir/$portsdir"
            ;;
        14 ) echo -e "\nConfiguring linux-x86_64 V0 without debug...\n"
            mkdir -p "$portsdir"
            mkdir -p aros-linux-x86_64-v0
            cd aros-linux-x86_64-v0
            "../$srcdir_v0/configure" --target=linux-x86_64 --with-portssources="$curdir/$portsdir"
            ;;
        15 ) echo -e "\nConfiguring pc-i386 V0...\n"
            mkdir -p "$portsdir"
            mkdir -p aros-pc-i386-v0
            cd aros-pc-i386-v0
            "../$srcdir_v0/configure" --target=pc-i386 --with-portssources="$curdir/$portsdir"
            ;;
        16 ) echo -e "\nConfiguring pc-x86_64 V0...\n"
            mkdir -p "$portsdir"
            mkdir -p aros-pc-x86_64-v0
            cd aros-pc-x86_64-v0
            "../$srcdir_v0/configure" --target=pc-x86_64 --with-portssources="$curdir/$portsdir"
            ;;


        0 ) exit 0
            ;;
    esac
done



cd "$curdir"

input=""
until [ "$input" = "9" ]
do
    cd "$curdir"
    echo -e "\nStep 4: Building"
    echo -e   "~~~~~~~~~~~~~~~~"
    echo -e "\nYou can only build what you've already configured."
    echo -e "\nABI V1 | ABI V0 |"
    echo -e   "-------+--------+----------------------------"
    echo -e   "   1   |   11   | linux-i386   (32-bit) debug"
    echo -e   "   2   |   12   | linux-i386   (32-bit)"
    echo -e   "   3   |   13   | linux-x86_64 (64-bit) debug"
    echo -e   "   4   |   14   | linux-x86_64 (64-bit)"
    echo -e   "   5   |   15   | pc-i386      (32-bit)"
    echo -e   "   6   |   16   | pc-x86_64    (64-bit)"
    echo -e "\n9 .. Leave loop, exit"

    echo -e "\nEnter number and press <Enter>:"

    read input
    case "$input" in
        1 ) echo -e "\nBuilding linux-i386 V1 with full debug...\n"
            cd aros-linux-i386-dbg
            make "-j"$cpucoresforcompile
            make default-x11keymaptable
            echo -e "\nIf everything went well AROS will be available"
            echo -e "in the directory aros-linux-i386-dbg/bin/<target>/AROS"
            ;;
        2 ) echo -e "\nBuilding linux-i386 V1 without debug...\n"
            cd aros-linux-i386
            make "-j"$cpucoresforcompile
            make default-x11keymaptable
            echo -e "\nIf everything went well AROS will be available"
            echo -e "in the directory aros-linux-i386/bin/<target>/AROS"
            ;;
        3 ) echo -e "\nBuilding linux-x86_64 V1 with full debug...\n"
            cd aros-linux-x86_64-dbg
            make "-j"$cpucoresforcompile
            make default-x11keymaptable
            echo -e "\nIf everything went well AROS will be available"
            echo -e "in the directory aros-linux-x86_64-dbg/bin/<target>/AROS"
            ;;
        4 ) echo -e "\nBuilding linux-x86_64 V1 without debug...\n"
            cd aros-linux-x86_64
            make "-j"$cpucoresforcompile
            make default-x11keymaptable
            echo -e "\nIf everything went well AROS will be available"
            echo -e "in the directory aros-linux-x86_64/bin/<target>/AROS"
            ;;
        5 ) echo -e "\nBuilding pc-i386 V1...\n"
            cd aros-pc-i386
            make "-j"$cpucoresforcompile
            make default-x11keymaptable
            echo -e "\nIf everything went well AROS will be available"
            echo -e "in the directory aros-pc-i386/bin/<target>/AROS"
            ;;
        6 ) echo -e "\nBuilding pc-x86_64 V1...\n"
            cd aros-pc-x86_64
            make "-j"$cpucoresforcompile
            make default-x11keymaptable
            echo -e "\nIf everything went well AROS will be available"
            echo -e "in the directory aros-pc-x86_64/bin/<target>/AROS"
            ;;


        11 ) echo -e "\nBuilding linux-i386 V0 with full debug...\n"
            cd aros-linux-i386-v0-dbg
            make "-j"$cpucoresforcompile
            make default-x11keymaptable
            echo -e "\nIf everything went well AROS will be available"
            echo -e "in the directory aros-linux-i386-v0-dbg/bin/<target>/AROS"
            ;;
        12 ) echo -e "\nBuilding linux-i386 V0 without debug...\n"
            cd aros-linux-i386-v0
            make "-j"$cpucoresforcompile
            make default-x11keymaptable
            echo -e "\nIf everything went well AROS will be available"
            echo -e "in the directory aros-linux-i386-v0/bin/<target>/AROS"
            ;;
        13 ) echo -e "\nBuilding linux-x86_64 V0 with full debug...\n"
            cd aros-linux-x86_64-v0-dbg
            make "-j"$cpucoresforcompile
            make default-x11keymaptable
            echo -e "\nIf everything went well AROS will be available"
            echo -e "in the directory aros-linux-x86_64-dbg/bin/<target>/AROS"
            ;;
        14 ) echo -e "\nBuilding linux-x86_64 V0 without debug...\n"
            cd aros-linux-x86_64-v0
            make "-j"$cpucoresforcompile
            make default-x11keymaptable
            echo -e "\nIf everything went well AROS will be available"
            echo -e "in the directory aros-linux-x86_64-v0/bin/<target>/AROS"
            ;;
        15 ) echo -e "\nBuilding pc-i386 V0...\n"
            cd aros-pc-i386-v0
            make "-j"$cpucoresforcompile
            make default-x11keymaptable
            echo -e "\nIf everything went well AROS will be available"
            echo -e "in the directory aros-pc-i386-v0/bin/<target>/AROS"
            ;;
        16 ) echo -e "\nBuilding pc-x86_64 V0...\n"
            cd aros-pc-x86_64-v0
            make "-j"$cpucoresforcompile
            make default-x11keymaptable
            echo -e "\nIf everything went well AROS will be available"
            echo -e "in the directory aros-pc-x86_64-v0/bin/<target>/AROS"
            ;;

    esac
done


cd "$curdir"

exit 0
