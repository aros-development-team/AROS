# Compilation configuration for building m68k-elf.
#

#
ifndef TOOLPREFIX
TOOLPREFIX=
endif
#
CC=	${TOOLPREFIX}gcc
LD=	${TOOLPREFIX}ld
STRIP=	${TOOLPREFIX}strip
OBJCOPY=${TOOLPREFIX}objcopy
NM=	${TOOLPREFIX}nm

COPTS+=	-DAH_BYTE_ORDER=AH_BIG_ENDIAN
