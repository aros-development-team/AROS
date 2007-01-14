#!/bin/sh

if [ ! -f /usr/include/asm/sigcontext.h ] ; then
    echo "Could not find asm/sigcontext.h"
    exit 20
fi

type=`${CC} -E /usr/include/asm/sigcontext.h | grep "^struct sigcontext" | sed 's/{//'`

handler=__sighandler_t
if [ ${CPU} = "m68k" ]; then
  sed "s/@sigcontext@/$type/" ${1-.}/../m68k/sigcore.h.src > ${2}
elif [ ${CPU} = "x86_64" ]; then
  sed "s/@sigcontext@/$type/;s/@sighandler@/$handler/" ${1-.}/../x86_64/sigcore.h.src > ${2}
elif [ ${CPU} = "ppc" ]; then
  sed "s/@sigcontext@/$type/;s/@sighandler@/$handler/" ${1-.}/../ppc/sigcore.h.src > ${2}
else
  sed "s/@sigcontext@/$type/;s/@sighandler@/$handler/" ${1-.}/sigcore.h.src > ${2}
fi
