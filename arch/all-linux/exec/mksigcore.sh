#!/bin/sh

type=`grep "^struct sigcontext" /usr/include/asm/sigcontext.h | sed 's/{//'`
handler=__sighandler_t
if [ `uname -m` = "m68k" ]; then
  sed "s/@sigcontext@/$type/" ${1-.}/../m68k/sigcore.h.src > ${2}
elif [ `uname -m` = "ppc" ]; then
  sed "s/@sigcontext@/$type/" ${1-.}/../ppc/sigcore.h.src > ${2}
else
  sed "s/@sigcontext@/$type/;s/@sighandler@/$handler/" ${1-.}/sigcore.h.src > ${2}
fi
