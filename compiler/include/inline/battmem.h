/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _INLINE_BATTMEM_H
#define _INLINE_BATTMEM_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef BATTMEM_BASE_NAME
#define BATTMEM_BASE_NAME BattMemBase
#endif

#define ObtainBattSemaphore() \
	LP0NR(0x6, ObtainBattSemaphore, \
	, BATTMEM_BASE_NAME)

#define ReadBattMem(buffer, offset, length) \
	LP3(0x12, ULONG, ReadBattMem, APTR, buffer, a0, unsigned long, offset, d0, unsigned long, length, d1, \
	, BATTMEM_BASE_NAME)

#define ReleaseBattSemaphore() \
	LP0NR(0xc, ReleaseBattSemaphore, \
	, BATTMEM_BASE_NAME)

#define WriteBattMem(buffer, offset, length) \
	LP3(0x18, ULONG, WriteBattMem, APTR, buffer, a0, unsigned long, offset, d0, unsigned long, length, d1, \
	, BATTMEM_BASE_NAME)

#endif /* _INLINE_BATTMEM_H */
