#!/bin/sh

type=`grep "^struct sigcontext" /usr/include/asm/sigcontext.h | sed 's/\\{//'`
if [ `uname -m` = "m68k" ]; then
  sed "s/@sigcontext@/$type/" ${1-.}/../m68k/sigcore.h.src > ${2}
else
  sed "s/@sigcontext@/$type/" ${1-.}/sigcore.h.src > ${2}
fi
