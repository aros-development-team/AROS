#!/bin/sh

if [ -f /usr/include/asm-${CPU}/sigcontext.h ]; then
    type=`grep "^struct sigcontext" /usr/include/asm-${CPU}/sigcontext.h | sed 's/{//'`
elif [ -f /usr/include/asm/sigcontext.h ]; then
    type=`grep "^struct sigcontext" /usr/include/asm/sigcontext.h | sed 's/{//'`
else
    echo "Could not find asm/sigcontext.h"
    exit 20
fi

handler=__sighandler_t
if [ `uname -m` = "m68k" ]; then
  sed "s/@sigcontext@/$type/" ${1-.}/../m68k/sigcore.h.src > ${2}
elif [ `uname -m` = "ppc" ]; then
  sed "s/@sigcontext@/$type/;s/@sighandler@/$handler/" ${1-.}/../ppc/sigcore.h.src > ${2}
else
  sed "s/@sigcontext@/$type/;s/@sighandler@/$handler/" ${1-.}/sigcore.h.src > ${2}
fi
