/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _INLINE_POTGO_H
#define _INLINE_POTGO_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef POTGO_BASE_NAME
#define POTGO_BASE_NAME PotgoBase
#endif

#define AllocPotBits(bits) \
	LP1(0x6, UWORD, AllocPotBits, unsigned long, bits, d0, \
	, POTGO_BASE_NAME)

#define FreePotBits(bits) \
	LP1NR(0xc, FreePotBits, unsigned long, bits, d0, \
	, POTGO_BASE_NAME)

#define WritePotgo(word, mask) \
	LP2NR(0x12, WritePotgo, unsigned long, word, d0, unsigned long, mask, d1, \
	, POTGO_BASE_NAME)

#endif /* _INLINE_POTGO_H */
