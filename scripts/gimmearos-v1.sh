#!/bin/bash

# Script for building AROS from the Subversion or GIT repository

# Currently only a limited amount of Linux distros is supported.
# If you improve this script, send modifications back to me, please.
# Matthias Rustler, mailto:mrustler@gmx.de

# This script is public domain. Use it at your own risk.

# $VER: gimmearos-v1.sh 1.14 (25.04.2019)

curdir="`pwd`"
srcdir="aros-src"
portsdir="$HOME/aros-ports-src"
tooldir="$HOME/aros-toolchain"
makeopts="-j3 -s"
configopts="--with-binutils-version=2.25 --with-gcc-version=6.3.0 --disable-sdl-hidd --with-portssources="$portsdir""

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


echo -e "\n\n\n\n\n"
echo -e "***********************************************"
echo -e "* Script for downloading and building of AROS *"
echo -e "***********************************************"
echo -e "\n\n"
echo -e "*********************************"
echo -e "* Step 1: install prerequisites *"
echo -e "*********************************"
echo -e "The build system needs some packages to do its job."
echo -e "If you are asked for a password enter you admin password."
echo -e "\n1 .. Get packages with apt-get for Debian and similar (e.g. Ubuntu)"
echo -e "     for building 32-bit AROS on 32-bit Linux or 64-bit-AROS on 64-bit-Linux"
echo -e "2 .. As 1) but with additional packages for building 32-bit AROS"
echo -e "     on 64-bit Linux"
echo -e "3 .. Get packages with yum for Fedora"
echo -e "4 .. Get packages with pacman for Arch"
echo -e "5 .. Get packages with zypper for openSuse"
echo -e "6 .. As 5) but with additional packages for building 32-bit AROS"
echo -e "     on 64-bit Linux"
echo -e "99 ..Skip this step"
echo -e "0 .. Exit"

echo -e "\nEnter number and press <Enter>:"

read input
case "$input" in
    1 ) echo -e "\nInstalling prerequisites with apt-get..."
        install_pkg "apt-get install" subversion
        install_pkg "apt-get install" git-core
        install_pkg "apt-get install" gcc
        install_pkg "apt-get install" g++
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
        install_pkg "apt-get install" libpng-dev
        install_pkg "apt-get install" libsdl1.2-dev
        install_pkg "apt-get install" byacc
        install_pkg "apt-get install" python-mako
        install_pkg "apt-get install" libxcursor-dev
        ;;

    2 ) echo -e "\nInstalling prerequisites with apt-get..."
        install_pkg "apt-get install" subversion
        install_pkg "apt-get install" git-core
        install_pkg "apt-get install" gcc
        install_pkg "apt-get install" g++
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
        install_pkg "apt-get install" libpng-dev
        install_pkg "apt-get install" gcc-multilib
        install_pkg "apt-get install" libsdl1.2-dev
        install_pkg "apt-get install" byacc
        install_pkg "apt-get install" python-mako
        install_pkg "apt-get install" libxcursor-dev

        install_pkg "apt-get install" libc6-dev-i386
        install_pkg "apt-get install" lib32gcc1
        install_pkg "apt-get install" ia32-libs
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

    5 ) echo -e "\nInstalling prerequisites with zypper..."
        # tools
        install_pkg "zypper --non-interactive install" subversion
        install_pkg "zypper --non-interactive install" git-core
        install_pkg "zypper --non-interactive install" gcc
        install_pkg "zypper --non-interactive install" gcc-c++
        install_pkg "zypper --non-interactive install" make
        install_pkg "zypper --non-interactive install" gawk
        install_pkg "zypper --non-interactive install" bison
        install_pkg "zypper --non-interactive install" flex
        install_pkg "zypper --non-interactive install" bzip2
        install_pkg "zypper --non-interactive install" netpbm
        install_pkg "zypper --non-interactive install" autoconf
        install_pkg "zypper --non-interactive install" automake
        install_pkg "zypper --non-interactive install" patch
        install_pkg "zypper --non-interactive install" cmake
        install_pkg "zypper --non-interactive install" gperf
        install_pkg "zypper --non-interactive install" perl-Switch
        install_pkg "zypper --non-interactive install" byacc

        # libs
        install_pkg "zypper --non-interactive install" libXxf86vm1
        install_pkg "zypper --non-interactive install" alsa-plugins-pulse

        #devel
        install_pkg "zypper --non-interactive install" libX11-devel
        install_pkg "zypper --non-interactive install" glibc-devel
        install_pkg "zypper --non-interactive install" libpng12-devel
        install_pkg "zypper --non-interactive install" alsa-devel
        ;;

    6 ) echo -e "\nInstalling prerequisites with zypper..."
        # tools
        install_pkg "zypper --non-interactive install" subversion
        install_pkg "zypper --non-interactive install" git-core
        install_pkg "zypper --non-interactive install" gcc
        install_pkg "zypper --non-interactive install" gcc-c++
        install_pkg "zypper --non-interactive install" make
        install_pkg "zypper --non-interactive install" gawk
        install_pkg "zypper --non-interactive install" bison
        install_pkg "zypper --non-interactive install" flex
        install_pkg "zypper --non-interactive install" bzip2
        install_pkg "zypper --non-interactive install" netpbm
        install_pkg "zypper --non-interactive install" autoconf
        install_pkg "zypper --non-interactive install" automake
        install_pkg "zypper --non-interactive install" patch
        install_pkg "zypper --non-interactive install" cmake
        install_pkg "zypper --non-interactive install" gperf
        install_pkg "zypper --non-interactive install" perl-Switch
        install_pkg "zypper --non-interactive install" byacc

        # libs
        install_pkg "zypper --non-interactive install" libXxf86vm1
        install_pkg "zypper --non-interactive install" alsa-plugins-pulse

        #devel
        install_pkg "zypper --non-interactive install" libX11-devel
        install_pkg "zypper --non-interactive install" glibc-devel
        install_pkg "zypper --non-interactive install" libpng12-devel

        # 32-bit support
        install_pkg "zypper --non-interactive install" gcc-32bit
        install_pkg "zypper --non-interactive install" gcc-c++-32bit
        install_pkg "zypper --non-interactive install" glibc-devel-32bit
        install_pkg "zypper --non-interactive install" libXxf86vm1-32bit
        install_pkg "zypper --non-interactive install" alsa-plugins-pulse-32bit
        install_pkg "zypper --non-interactive install" alsa-devel-32bit
        ;;

    0 ) exit 0
        ;;
esac

cd "$curdir"

input=""
until [ "$input" = "99" ]
do
    echo -e "\n\n\n\n\n"
    echo -e "***********************************************"
    echo -e "* Step 2: get the sources from the repository *"
    echo -e "***********************************************"
    echo -e "\nYou can either use Subversion or Git. Git doesn't require"
    echo -e   "a password, but you'll get only read-only access."
    echo -e   "The repositories will be checked out into the current directory."
    echo -e "\nABI V1 with | ABI V1   |"
    echo -e   "Subversion  | with GIT |       Content"
    echo -e   "------------+----------+--------------------------"
    echo -e   "     1      |    21    | Get AROS core (required)"
    echo -e   "     2      |    22    | Get contrib (optional)"
    echo -e "\n     3      |    23    | Get ports source (optional, needs contrib)"
    echo -e "\n     4      |    ---   | Get documentation source (optional)"
    echo -e   "     5      |    ---   | Get binaries (wallpapers, logos etc.) (optional)"
    echo -e "\n99 .. Go to next step"
    echo -e   "0  .. Exit"

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
until [ "$input" = "99" ]
do
    cd "$curdir"

    echo -e "\n\n\n\n\n"
    echo -e "*******************************"
    echo -e "* Step 3: configure toolchain *"
    echo -e "*******************************"
    echo -e "\n  1    | i386   (32-bit)"
    echo -e   "  2    | x86_64 (64-bit)"
    echo -e   "  3    | m68k   (32-bit)"
    echo -e   "  4    | armhf  (32-bit)"
    echo -e "\n99 .. Go to next step"
    echo -e   "0  .. Exit"

    echo -e "\nEnter number and press <Enter>:"

    read input
    case "$input" in
        1 ) echo -e "\nConfiguring i386 Toolchain...\n"
            mkdir -p "$portsdir"
            mkdir -p aros-i386-toolchain-builddir
            cd aros-i386-toolchain-builddir
            "../$srcdir/configure" $configopts --target=linux-i386 --with-aros-toolchain-install="$tooldir"-i386
            ;;
        2 ) echo -e "\nConfiguring x86_64 Toolchain...\n"
            mkdir -p "$portsdir"
            mkdir -p aros-x86_64-toolchain-builddir
            cd aros-x86_64-toolchain-builddir
            "../$srcdir/configure" $configopts --target=linux-x86_64 --with-aros-toolchain-install="$tooldir"-x86_64
            ;;
        3 ) echo -e "\nConfiguring m68k Toolchain...\n"
            mkdir -p "$portsdir"
            mkdir -p aros-m68k-toolchain-builddir
            cd aros-m68k-toolchain-builddir
            "../$srcdir/configure" $configopts --target=amiga-m68k --with-aros-toolchain-install="$tooldir"-m68k
            ;;
        4 ) echo -e "\nConfiguring armhf Toolchain...\n"
            mkdir -p "$portsdir"
            mkdir -p aros-armhf-toolchain-builddir
            cd aros-armhf-toolchain-builddir
            "../$srcdir/configure" $configopts --target=raspi-armhf --with-aros-toolchain-install="$tooldir"-armhf
            ;;

        0 ) exit 0
            ;;
    esac
done


cd "$curdir"

input=""
until [ "$input" = "99" ]
do
    cd "$curdir"

    echo -e "\n\n\n\n\n"
    echo -e "***************************"
    echo -e "* Step 4: build toolchain *"
    echo -e "***************************"
    echo -e "\nYou can only build what you've already configured."
    echo -e "\n   1   | i386   (32-bit)"
    echo -e   "   2   | x86_64 (64-bit)"
    echo -e   "   3   | m68k   (32-bit)"
    echo -e   "   4   | armhf  (32-bit)"
    echo -e "\n9 .. Go to next step"
    echo -e   "0 .. Exit"
    echo -e "\nEnter number and press <Enter>:"

    read input
    case "$input" in
        1 ) echo -e "\nBuilding i386 Toolchain...\n"
            cd aros-i386-toolchain-builddir
            make $makeopts crosstools
            cd "$curdir"
            rm -rf aros-i386-toolchain-builddir
            ;;
        2 ) echo -e "\nBuilding x86_64 Toolchain...\n"
            cd aros-x86_64-toolchain-builddir
            make $makeopts crosstools
            cd "$curdir"
            rm -rf aros-x86_64-toolchain-builddir
            ;;
        3 ) echo -e "\nBuilding m68k Toolchain...\n"
            cd aros-m68k-toolchain-builddir
            make $makeopts crosstools
            cd "$curdir"
            rm -rf aros-m68k-toolchain-builddir
            ;;
        4 ) echo -e "\nBuilding armhf Toolchain...\n"
            cd aros-armhf-toolchain-builddir
            make $makeopts crosstools
            cd "$curdir"
            rm -rf aros-armhf-toolchain-builddir
            ;;

        0 ) exit 0
            ;;
    esac
done


cd "$curdir"

input=""
until [ "$input" = "99" ]
do
    cd "$curdir"

    echo -e "\n\n\n\n\n"
    echo -e "**************************"
    echo -e "* Step 5: configure AROS *"
    echo -e "**************************"
    echo -e "\n  1    | linux-i386    (32-bit) debug"
    echo -e   "  2    | linux-i386    (32-bit)"
    echo -e   "  3    | linux-x86_64  (64-bit) debug"
    echo -e   "  4    | linux-x86_64  (64-bit)"
    echo -e   "  5    | pc-i386       (32-bit)"
    echo -e   "  6    | pc-x86_64     (64-bit)"
    echo -e   "  7    | pc-x86_64 SMP (64-bit)"
    echo -e   "  8    | amiga-m68k    (32-bit)"
    echo -e   "  9    | raspi-armhf   (32-bit)"
    echo -e "\n99 .. Go to next step"
    echo -e   "0  .. Exit"

    echo -e "\nEnter number and press <Enter>:"

    read input
    case "$input" in
        1 ) echo -e "\nConfiguring linux-i386 V1 with full debug...\n"
            mkdir -p "$portsdir"
            mkdir -p aros-linux-i386-dbg
            cd aros-linux-i386-dbg
            "../$srcdir/configure" $configopts --target=linux-i386 --enable-debug --with-aros-toolchain-install="$tooldir"-i386 --with-aros-toolchain=yes
            ;;
        2 ) echo -e "\nConfiguring linux-i386 V1 without debug...\n"
            mkdir -p "$portsdir"
            mkdir -p aros-linux-i386
            cd aros-linux-i386
            "../$srcdir/configure" $configopts --target=linux-i386 --with-aros-toolchain-install="$tooldir"-i386 --with-aros-toolchain=yes
            ;;
        3 ) echo -e "\nConfiguring linux-x86_64 V1 with full debug...\n"
            mkdir -p "$portsdir"
            mkdir -p aros-linux-x86_64-dbg
            cd aros-linux-x86_64-dbg
            "../$srcdir/configure" $configopts --target=linux-x86_64 --enable-debug --with-aros-toolchain-install="$tooldir"-x86_64 --with-aros-toolchain=yes
            ;;
        4 ) echo -e "\nConfiguring linux-x86_64 V1 without debug...\n"
            mkdir -p "$portsdir"
            mkdir -p aros-linux-x86_64
            cd aros-linux-x86_64
            "../$srcdir/configure" $configopts --target=linux-x86_64 --with-aros-toolchain-install="$tooldir"-x86_64 --with-aros-toolchain=yes
            ;;
        5 ) echo -e "\nConfiguring pc-i386 V1...\n"
            mkdir -p "$portsdir"
            mkdir -p aros-pc-i386
            cd aros-pc-i386
            "../$srcdir/configure" $configopts --target=pc-i386 --with-aros-toolchain-install="$tooldir"-i386 --with-aros-toolchain=yes
            ;;
        6 ) echo -e "\nConfiguring pc-x86_64 V1...\n"
            mkdir -p "$portsdir"
            mkdir -p aros-pc-x86_64
            cd aros-pc-x86_64
            "../$srcdir/configure" $configopts --target=pc-x86_64 --with-aros-toolchain-install="$tooldir"-x86_64 --with-aros-toolchain=yes
            ;;
        7 ) echo -e "\nConfiguring pc-x86_64 SMP V1...\n"
            mkdir -p "$portsdir"
            mkdir -p aros-pc-x86_64-smp
            cd aros-pc-x86_64-smp
            "../$srcdir/configure" $configopts --target=pc-x86_64 --enable-target-variant=smp --with-aros-toolchain-install="$tooldir"-x86_64 --with-aros-toolchain=yes
            ;;
        8 ) echo -e "\nConfiguring amiga-m68k V1...\n"
            mkdir -p "$portsdir"
            mkdir -p aros-amiga-m68k
            cd aros-amiga-m68k
            "../$srcdir/configure" $configopts --target=amiga-m68k --with-serial-debug=yes --with-aros-toolchain-install="$tooldir"-m68k --with-aros-toolchain=yes
            ;;
        9 ) echo -e "\nConfiguring raspi-armhf V1...\n"
            mkdir -p "$portsdir"
            mkdir -p aros-raspi-armhf
            cd aros-raspi-armhf
            "../$srcdir/configure" $configopts --target=raspi-armhf --with-aros-toolchain-install="$tooldir"-armhf --with-aros-toolchain=yes
            ;;

        0 ) exit 0
            ;;
    esac
done


cd "$curdir"

input=""
until [ "$input" = "99" ]
do
    cd "$curdir"

	echo -e "\n\n\n\n\n"
    echo -e "*****************"
    echo -e "* Step 6: build *"
    echo -e "*****************"
    echo -e "\nYou can only build what you've already configured."
    echo -e "\n   1   | linux-i386    (32-bit) debug"
    echo -e   "   2   | linux-i386    (32-bit)"
    echo -e   "   3   | linux-x86_64  (64-bit) debug"
    echo -e   "   4   | linux-x86_64  (64-bit)"
    echo -e   "   5   | pc-i386       (32-bit)"
    echo -e   "   6   | pc-x86_64     (64-bit)"
    echo -e   "   7   | pc-x86_64 SMP (64-bit)"
    echo -e   "   8   | amiga-m68k    (32-bit)"
    echo -e   "   9   | raspi-armhf   (32-bit)"
    echo -e "\n99 .. Go to next step"
    echo -e   "0  .. Exit"
    echo -e "\nEnter number and press <Enter>:"

    read input
    case "$input" in
        1 ) echo -e "\nBuilding linux-i386 V1 with full debug...\n"
            cd aros-linux-i386-dbg
            make $makeopts
            make $makeopts contrib
            make $makeopts ports
            echo -e "\nIf everything went well AROS will be available"
            echo -e "in the directory aros-linux-i386-dbg/bin/<target>/AROS"
            ;;
        2 ) echo -e "\nBuilding linux-i386 V1 without debug...\n"
            cd aros-linux-i386
            make $makeopts
            make $makeopts contrib
            make $makeopts ports
            echo -e "\nIf everything went well AROS will be available"
            echo -e "in the directory aros-linux-i386/bin/<target>/AROS"
            ;;
        3 ) echo -e "\nBuilding linux-x86_64 V1 with full debug...\n"
            cd aros-linux-x86_64-dbg
            make $makeopts
            make $makeopts contrib
            make $makeopts ports
            echo -e "\nIf everything went well AROS will be available"
            echo -e "in the directory aros-linux-x86_64-dbg/bin/<target>/AROS"
            ;;
        4 ) echo -e "\nBuilding linux-x86_64 V1 without debug...\n"
            cd aros-linux-x86_64
            make $makeopts
            make $makeopts contrib
            make $makeopts ports
            echo -e "\nIf everything went well AROS will be available"
            echo -e "in the directory aros-linux-x86_64/bin/<target>/AROS"
            ;;
        5 ) echo -e "\nBuilding pc-i386 V1...\n"
            cd aros-pc-i386
            make $makeopts
            make $makeopts contrib
            make $makeopts ports
            make $makeopts distfiles
            echo -e "\nIf everything went well AROS will be available"
            echo -e "in the directory aros-pc-i386/distfiles"
            ;;
        6 ) echo -e "\nBuilding pc-x86_64 V1...\n"
            cd aros-pc-x86_64
            make $makeopts
            make $makeopts contrib
            make $makeopts ports
            make $makeopts distfiles
            echo -e "\nIf everything went well AROS will be available"
            echo -e "in the directory aros-pc-x86_64/distfiles"
            ;;
        7 ) echo -e "\nBuilding pc-x86_64 SMP V1...\n"
            cd aros-pc-x86_64-smp
            make $makeopts
            make $makeopts contrib
            make $makeopts ports
            make $makeopts distfiles
            echo -e "\nIf everything went well AROS will be available"
            echo -e "in the directory aros-pc-x86_64-smp/distfiles"
            ;;
        8 ) echo -e "\nBuilding amiga-m68k V1...\n"
            cd aros-amiga-m68k
            make $makeopts
            make $makeopts contrib
            make $makeopts ports
            make $makeopts distfiles
            echo -e "\nIf everything went well AROS will be available"
            echo -e "in the directory aros-amiga-m68k/distfiles"
            ;;
        9 ) echo -e "\nBuilding raspi-armhf V1...\n"
            cd aros-raspi-armhf
            make $makeopts
            make $makeopts contrib
            make $makeopts ports
            make $makeopts distfiles
            echo -e "\nIf everything went well AROS will be available"
            echo -e "in the directory aros-raspi-armhf/distfiles"
            ;;

        0 ) exit 0
            ;;
    esac
done

cd "$curdir"

exit 0
