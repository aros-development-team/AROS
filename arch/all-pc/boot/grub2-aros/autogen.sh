#! /usr/bin/env bash

set -e

export LC_COLLATE=C
unset LC_ALL

find . -iname '*.[ch]' ! -ipath './grub-core/lib/libgcrypt-grub/*' ! -ipath './build-aux/*' ! -ipath './grub-core/lib/libgcrypt/src/misc.c' ! -ipath './grub-core/lib/libgcrypt/src/global.c' ! -ipath './grub-core/lib/libgcrypt/src/secmem.c'  ! -ipath './util/grub-gen-widthspec.c' ! -ipath './util/grub-gen-asciih.c' |sort > po/POTFILES.in
find util -iname '*.in' ! -name Makefile.in  |sort > po/POTFILES-shell.in

echo "Importing unicode..."
python util/import_unicode.py unicode/UnicodeData.txt unicode/BidiMirroring.txt unicode/ArabicShaping.txt grub-core/unidata.c

echo "Importing libgcrypt..."
python util/import_gcry.py grub-core/lib/libgcrypt/ grub-core
sed -n -f util/import_gcrypth.sed < grub-core/lib/libgcrypt/src/gcrypt.h.in > include/grub/gcrypt/gcrypt.h
if [ -f include/grub/gcrypt/g10lib.h ]; then
    rm include/grub/gcrypt/g10lib.h
fi
if [ -d grub-core/lib/libgcrypt-grub/mpi/generic ]; then 
    rm -rf grub-core/lib/libgcrypt-grub/mpi/generic
fi
ln -s ../../../grub-core/lib/libgcrypt-grub/src/g10lib.h include/grub/gcrypt/g10lib.h
cp -R grub-core/lib/libgcrypt/mpi/generic grub-core/lib/libgcrypt-grub/mpi/generic

for x in mpi-asm-defs.h mpih-add1.c mpih-sub1.c mpih-mul1.c mpih-mul2.c mpih-mul3.c mpih-lshift.c mpih-rshift.c; do
    if [ -h grub-core/lib/libgcrypt-grub/mpi/"$x" ] || [ -f grub-core/lib/libgcrypt-grub/mpi/"$x" ]; then
	rm grub-core/lib/libgcrypt-grub/mpi/"$x"
    fi
    ln -s generic/"$x" grub-core/lib/libgcrypt-grub/mpi/"$x"
done

echo "Generating Automake input..."

# Automake doesn't like including files from a path outside the project.
rm -f contrib grub-core/contrib
if [ "x${GRUB_CONTRIB}" != x ]; then
  [ "${GRUB_CONTRIB}" = contrib ] || ln -s "${GRUB_CONTRIB}" contrib
  [ "${GRUB_CONTRIB}" = grub-core/contrib ] || ln -s ../contrib grub-core/contrib
fi

UTIL_DEFS='Makefile.util.def Makefile.utilgcry.def'
CORE_DEFS='grub-core/Makefile.core.def grub-core/Makefile.gcry.def'

for extra in contrib/*/Makefile.util.def; do
  if test -e "$extra"; then
    UTIL_DEFS="$UTIL_DEFS $extra"
  fi
done

for extra in contrib/*/Makefile.core.def; do
  if test -e "$extra"; then
    CORE_DEFS="$CORE_DEFS $extra"
  fi
done

python gentpl.py $UTIL_DEFS > Makefile.util.am
python gentpl.py $CORE_DEFS > grub-core/Makefile.core.am

for extra in contrib/*/Makefile.common; do
  if test -e "$extra"; then
    echo "include $extra" >> Makefile.util.am
    echo "include $extra" >> grub-core/Makefile.core.am
  fi
done

for extra in contrib/*/Makefile.util.common; do
  if test -e "$extra"; then
    echo "include $extra" >> Makefile.util.am
  fi
done

for extra in contrib/*/Makefile.core.common; do
  if test -e "$extra"; then
    echo "include $extra" >> grub-core/Makefile.core.am
  fi
done

echo "Saving timestamps..."
echo timestamp > stamp-h.in

echo "Running autoreconf..."
autoreconf -vi
exit 0
