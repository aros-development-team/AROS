#!/bin/sh -x

PREFIX="$1"
if [ -z "$PREFIX" ]; then
	PREFIX="/opt/m68k-elf"
fi

PATH=$PATH:`pwd`/fake
export PATH

BINUTILS_VERSION=2.22
GCC_VERSION=4.6.2
GMP_VERSION=5.0.1
MPFR_VERSION=3.0.1
MPC_VERSION=0.9
GDB_VERSION=7.3.1
NEWLIB_VERSION=1.19.0

fetch () {
	mkdir -p download || exit 1
	if [ ! -f download/`basename "$1"` ]; then
		wget -c -O download/`basename "$1"` "$1" || exit 1
	fi
}

unpack () {
	mkdir -p src || exit 1
	unpackSRC="$1".tar.bz2
	if [ -z "$2" ]; then
		unpackDST="$1"
	else
		unpackDST="$2"
	fi
	if [ -n "$STERILIZE" -o ! -f "src/$unpackDST/configure" ]; then
 		rm -rf "src/${unpackDST}"
		tar -C src -jxvf `pwd`/download/"${unpackSRC}" || exit 1
		HERE=`pwd`
		for d in `pwd`/download/"$unpackDST"-*.patch; do
		    if [ -f "$d" ]; then
			cd "src/${unpackDST}"
			patch -p1 <$d || exit 1
			cd "$HERE"
		    fi
		done
	fi
	[ -f "src/$unpackDST/configure" ] || exit 1
}

config () {
	[ -n "${STERILIZE}" ] && rm -rf "build/${configBASE}"
	[ -e build/.$1.config -a build/.$1.config -nt src/$1-$2/configure ] && return 0
	configBASE=$1
	shift
	configVERSION=$1
	shift

	mkdir -p build/${configBASE}
	(cd build/${configBASE}
	 ../../src/${configBASE}-${configVERSION}/configure  \
		--without-headers \
		--disable-threads \
		--disable-libssp \
		--disable-nls \
		--disable-shared \
		--disable-lto \
		--without-pic \
		--with-gmp="${PREFIX}" \
		--with-mpfr="${PREFIX}" \
		--with-mpc="${PREFIX}" \
		--prefix="${PREFIX}" \
		"$@" || exit 1
	) || exit $?
	touch build/.${configBASE}.config
}

build () {
	[ -e build/.$1.build -a build/.$1.build -nt build/.$1.config ] && return 0
	make -C build/$1 || exit 1
	touch build/.$1.build
}

deploy () {
	[ -e build/.$1.deploy -a build/.$1.deploy -nt build/.$1.build ] && return 0
	make -C build/$1 install || exit 1
	touch build/.$1.deploy
}

fetch ftp://ftp.gnu.org/pub/gnu/binutils/binutils-${BINUTILS_VERSION}.tar.bz2
fetch ftp://ftp.gnu.org/pub/gnu/gcc/gcc-${GCC_VERSION}/gcc-core-${GCC_VERSION}.tar.bz2
fetch ftp://ftp.gnu.org/pub/gnu/gcc/gcc-${GCC_VERSION}/gcc-g++-${GCC_VERSION}.tar.bz2
fetch ftp://ftp.gnu.org/pub/gnu/gcc/gcc-${GCC_VERSION}/gcc-objc-${GCC_VERSION}.tar.bz2
fetch ftp://ftp.gnu.org/pub/gnu/gmp/gmp-${GMP_VERSION}.tar.bz2
fetch ftp://ftp.gnu.org/pub/gnu/mpfr/mpfr-${MPFR_VERSION}.tar.bz2
fetch ftp://ftp.gnu.org/pub/gnu/gdb/gdb-${GDB_VERSION}.tar.bz2
fetch http://www.evillabs.net/AROS/gcc-4.6.2-aros.patch
fetch http://www.multiprecision.org/mpc/download/mpc-${MPC_VERSION}.tar.gz
if [ download/mpc-${MPC_VERSION}.tar.gz -nt download/mpc-${MPC_VERSION}.tar.bz2 ]; then
	(gzip -dc download/mpc-${MPC_VERSION}.tar.gz | bzip2 -c >download/mpc-${MPC_VERSION}.tar.bz2) || exit 1
fi
fetch ftp://sources.redhat.com/pub/newlib/newlib-${NEWLIB_VERSION}.tar.gz
if [ download/newlib-${NEWLIB_VERSION}.tar.gz -nt download/newlib-${NEWLIB_VERSION}.tar.bz2 ]; then
	(gzip -dc download/newlib-${NEWLIB_VERSION}.tar.gz | bzip2 -c >download/newlib-${NEWLIB_VERSION}.tar.bz2) || exit 1
fi

unpack binutils-${BINUTILS_VERSION}
unpack gcc-core-${GCC_VERSION} gcc-${GCC_VERSION}
unpack gcc-objc-${GCC_VERSION} gcc-${GCC_VERSION}
unpack gcc-g++-${GCC_VERSION} gcc-${GCC_VERSION}
unpack gmp-${GMP_VERSION}
unpack mpfr-${MPFR_VERSION}
unpack mpc-${MPC_VERSION}
unpack gdb-${GDB_VERSION}
unpack newlib-${NEWLIB_VERSION}

config binutils ${BINUTILS_VERSION} \
		--target=m68k-elf \
		--program-prefix=m68k-elf- \
		--enable-languages=c 
build binutils
deploy binutils

config gmp ${GMP_VERSION}
build gmp
deploy gmp

config mpfr ${MPFR_VERSION}
build mpfr
deploy mpfr

config mpc ${MPC_VERSION}
build mpc
deploy mpc

config gcc ${GCC_VERSION} \
		--target=m68k-elf \
		--program-prefix=m68k-elf- \
		--without-target-libiberty \
		--without-target-zlib \
		--disable-bootstrap \
		--enable-languages="c"
build gcc
deploy gcc

config gdb ${GDB_VERSION} \
		--target=m68k-elf \
		--program-prefix=m68k-elf-
build gdb
deploy gdb

# Minimal C library
config newlib ${NEWLIB_VERSION} \
		--target=m68k-elf \
		--program-prefix=m68k-elf- \
		--enable-languages="c"
build newlib
deploy newlib

# Build C++ and ObjC
config gcc ${GCC_VERSION} \
		--with-newlib \
		--target=m68k-elf \
		--program-prefix=m68k-elf- \
		--disable-bootstrap \
		--enable-languages="c c++ objc"
build gcc
deploy gcc

