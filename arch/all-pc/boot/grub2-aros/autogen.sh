#! /usr/bin/env bash

set -e

export LC_CTYPE=C
export LC_COLLATE=C
unset LC_ALL

autogen --version >/dev/null || exit 1

echo "Importing unicode..."
python util/import_unicode.py unicode/UnicodeData.txt unicode/BidiMirroring.txt unicode/ArabicShaping.txt grub-core/unidata.c

echo "Importing libgcrypt..."
python util/import_gcry.py grub-core/lib/libgcrypt/ grub-core

echo "Creating Makefile.tpl..."
python gentpl.py | sed -e '/^$/{N;/^\n$/D;}' > Makefile.tpl

echo "Running autogen..."

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

cat $UTIL_DEFS | autogen -T Makefile.tpl | sed -e '/^$/{N;/^\n$/D;}' > Makefile.util.am
cat $CORE_DEFS | autogen -T Makefile.tpl | sed -e '/^$/{N;/^\n$/D;}' > grub-core/Makefile.core.am

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
