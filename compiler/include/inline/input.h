/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _INLINE_INPUT_H
#define _INLINE_INPUT_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef INPUT_BASE_NAME
#define INPUT_BASE_NAME InputBase
#endif

#define PeekQualifier() \
	LP0(0x2a, UWORD, PeekQualifier, \
	, INPUT_BASE_NAME)

#endif /* _INLINE_INPUT_H */
