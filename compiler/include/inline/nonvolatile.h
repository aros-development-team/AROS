/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _INLINE_NONVOLATILE_H
#define _INLINE_NONVOLATILE_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef NONVOLATILE_BASE_NAME
#define NONVOLATILE_BASE_NAME NVBase
#endif

#define DeleteNV(appName, itemName, killRequesters) \
	LP3(0x30, BOOL, DeleteNV, STRPTR, appName, a0, STRPTR, itemName, a1, long, killRequesters, d1, \
	, NONVOLATILE_BASE_NAME)

#define FreeNVData(data) \
	LP1NR(0x24, FreeNVData, APTR, data, a0, \
	, NONVOLATILE_BASE_NAME)

#define GetCopyNV(appName, itemName, killRequesters) \
	LP3(0x1e, APTR, GetCopyNV, STRPTR, appName, a0, STRPTR, itemName, a1, long, killRequesters, d1, \
	, NONVOLATILE_BASE_NAME)

#define GetNVInfo(killRequesters) \
	LP1(0x36, struct NVInfo *, GetNVInfo, long, killRequesters, d1, \
	, NONVOLATILE_BASE_NAME)

#define GetNVList(appName, killRequesters) \
	LP2(0x3c, struct MinList *, GetNVList, STRPTR, appName, a0, long, killRequesters, d1, \
	, NONVOLATILE_BASE_NAME)

#define SetNVProtection(appName, itemName, mask, killRequesters) \
	LP4(0x42, BOOL, SetNVProtection, STRPTR, appName, a0, STRPTR, itemName, a1, long, mask, d2, long, killRequesters, d1, \
	, NONVOLATILE_BASE_NAME)

#define StoreNV(appName, itemName, data, length, killRequesters) \
	LP5(0x2a, UWORD, StoreNV, STRPTR, appName, a0, STRPTR, itemName, a1, APTR, data, a2, unsigned long, length, d0, long, killRequesters, d1, \
	, NONVOLATILE_BASE_NAME)

#endif /* _INLINE_NONVOLATILE_H */
