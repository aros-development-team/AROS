#ifndef _INLINE_AROS_H
#define _INLINE_AROS_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef AROS_BASE_NAME
#define AROS_BASE_NAME ArosBase
#endif

#define ArosInquire(query) \
	LP1(0x1e, IPTR, ArosInquire, ULONG, query, d0, \
	, AROS_BASE_NAME)

#endif /* _INLINE_AROS_H */
