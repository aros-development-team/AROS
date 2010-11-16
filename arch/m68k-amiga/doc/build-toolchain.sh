#!/bin/sh -x

PREFIX="$1"
if [ -z "$PREFIX" ]; then
	PREFIX="/opt/m68k-elf"
fi

PATH=$PATH:`pwd`/fake
export PATH

BINUTILS_VERSION=2.20.1
GCC_VERSION=4.5.1
GMP_VERSION=4.3.2
MPFR_VERSION=2.4.2
MPC_VERSION=0.8.1

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
		for d in `pwd`/download/"$1"-*.patch; do
			cd "src/${unpackDST}"
			patch -p1 <$d || exit 1
			cd "$HERE"
		done
	fi
	[ -f "src/$unpackDST/configure" ] || exit 1
}

config () {
	configBASE=$1
	shift
	configVERSION=$1
	shift

	[ -n "${STERILIZE}" ] && rm -rf "build/${configBASE}"
	mkdir -p build/${configBASE}
	[ ! -f build/${configBASE}/Makefile ] && (cd build/${configBASE}
	 ../../src/${configBASE}-${configVERSION}/configure  \
		--without-headers \
		--disable-threads \
		--disable-libssp \
		--disable-nls \
		--disable-shared \
		--without-pic \
		--with-gmp="${PREFIX}" \
		--with-mpfr="${PREFIX}" \
		--with-mpc="${PREFIX}" \
		--prefix="${PREFIX}" \
		"$@" || exit 1
	)
}

build () {
	make -C build/$1 || exit 1
}

deploy () {
	make -C build/$1 install || exit 1
}

fetch ftp://ftp.gnu.org/pub/gnu/binutils/binutils-${BINUTILS_VERSION}.tar.bz2
fetch ftp://ftp.gnu.org/pub/gnu/gcc/gcc-${GCC_VERSION}/gcc-core-${GCC_VERSION}.tar.bz2
fetch ftp://ftp.gnu.org/pub/gnu/gmp/gmp-${GMP_VERSION}.tar.bz2
fetch ftp://ftp.gnu.org/pub/gnu/mpfr/mpfr-${MPFR_VERSION}.tar.bz2
fetch http://www.evillabs.net/AROS/gcc-4.5.1-amiga-elf.patch
fetch ftp://gcc.gnu.org/pub/gcc/infrastructure/mpc-${MPC_VERSION}.tar.gz
(gzip -dc download/mpc-${MPC_VERSION}.tar.gz | bzip2 -c >download/mpc-${MPC_VERSION}.tar.bz2) || exit 1

unpack binutils-${BINUTILS_VERSION}
unpack gcc-core-${GCC_VERSION} gcc-${GCC_VERSION}
unpack gmp-${GMP_VERSION}
unpack mpfr-${MPFR_VERSION}
unpack mpc-${MPC_VERSION}

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
		--enable-languages=c 
build gcc
deploy gcc
