/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _INLINE_DISK_H
#define _INLINE_DISK_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef DISK_BASE_NAME
#define DISK_BASE_NAME DiskBase
#endif

#define AllocUnit(unitNum) \
	LP1(0x6, BOOL, AllocUnit, long, unitNum, d0, \
	, DISK_BASE_NAME)

#define FreeUnit(unitNum) \
	LP1NR(0xc, FreeUnit, long, unitNum, d0, \
	, DISK_BASE_NAME)

#define GetUnit(unitPointer) \
	LP1(0x12, struct DiskResourceUnit *, GetUnit, struct DiskResourceUnit *, unitPointer, a1, \
	, DISK_BASE_NAME)

#define GetUnitID(unitNum) \
	LP1(0x1e, LONG, GetUnitID, long, unitNum, d0, \
	, DISK_BASE_NAME)

#define GiveUnit() \
	LP0NR(0x18, GiveUnit, \
	, DISK_BASE_NAME)

#define ReadUnitID(unitNum) \
	LP1(0x24, LONG, ReadUnitID, long, unitNum, d0, \
	, DISK_BASE_NAME)

#endif /* _INLINE_DISK_H */
