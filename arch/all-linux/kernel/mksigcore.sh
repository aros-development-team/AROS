#!/bin/sh

sigcontextpath=/usr/include/bits/sigcontext.h
if [ ! -f $sigcontextpath ] ; then
    sigcontextpath=/usr/include/${CPU}-linux-gnu/bits/sigcontext.h
    if [ ! -f $sigcontextpath ] ; then
        echo "Could not find bits/sigcontext.h"
        exit 20
    fi
fi

type=`${CC} -D_SIGNAL_H -E ${sigcontextpath} | grep "^struct sigcontext" | sed 's/{//'`
handler=__sighandler_t

sed "s/@sigcontext@/$type/;s/@sighandler@/$handler/" ${1-.}/sigcore.h.${CPU}.src > ${2}
