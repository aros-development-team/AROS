#ifndef _INLINE_MISC_H
#define _INLINE_MISC_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef MISC_BASE_NAME
#define MISC_BASE_NAME MiscBase
#endif

#define AllocMiscResource(unitNum, name) \
	LP2(0x6, UBYTE *, AllocMiscResource, unsigned long, unitNum, d0, UBYTE *, name, a1, \
	, MISC_BASE_NAME)

#define FreeMiscResource(unitNum) \
	LP1NR(0xc, FreeMiscResource, unsigned long, unitNum, d0, \
	, MISC_BASE_NAME)

#endif /* _INLINE_MISC_H */
