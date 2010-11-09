#!/bin/sh

if [ -z "$1" ]; then
	TARGET=amiga-m68k-eabi
	OPTS="-Os"
else
	TARGET="$1"
	shift
	OPTS="$@"
fi

(cd ../AROS; autoconf) || exit 1

DOWNLOAD=`pwd`/../AROS/Sources
mkdir -p "${DOWNLOAD}" || exit 1

../AROS/configure --target="${TARGET}" \
             --with-optimization="${OPTS}" \
             --with-serial-debug \
             --with-portssources="${DOWNLOAD}"
