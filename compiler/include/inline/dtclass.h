/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _INLINE_DTCLASS_H
#define _INLINE_DTCLASS_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef DTCLASS_BASE_NAME
#define DTCLASS_BASE_NAME DTClassBase
#endif

#define ObtainEngine() \
	LP0(0x1e, Class *, ObtainEngine, \
	, DTCLASS_BASE_NAME)

#endif /* _INLINE_DTCLASS_H */
