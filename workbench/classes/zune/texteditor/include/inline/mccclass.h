#ifndef _INLINE_MCCCLASS_H
#define _INLINE_MCCCLASS_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef MCCCLASS_BASE_NAME
#define MCCCLASS_BASE_NAME MCCClassBase
#endif

#define MCC_Query(which) \
	LP1(0x1e, ULONG, MCC_Query, LONG, which, d0, \
	, MCCCLASS_BASE_NAME)

#endif /*  _INLINE_MCCCLASS_H  */
