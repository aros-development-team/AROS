/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _INLINE_RAMDRIVE_H
#define _INLINE_RAMDRIVE_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef RAMDRIVE_BASE_NAME
#define RAMDRIVE_BASE_NAME RamdriveDevice
#endif

#define KillRAD(unit) \
	LP1(0x30, STRPTR, KillRAD, unsigned long, unit, d0, \
	, RAMDRIVE_BASE_NAME)

#define KillRAD0() \
	LP0(0x2a, STRPTR, KillRAD0, \
	, RAMDRIVE_BASE_NAME)

#endif /* _INLINE_RAMDRIVE_H */
