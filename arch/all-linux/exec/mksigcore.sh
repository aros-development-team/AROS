#!/bin/sh

type=`grep "^struct sigcontext" /usr/include/asm/sigcontext.h | sed 's/\\{//'`
sed "s/@sigcontext@/$type/" ${1-.}/sigcore.h.src > ${2}
