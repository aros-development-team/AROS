/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _INLINE_TRANSLATOR_H
#define _INLINE_TRANSLATOR_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef TRANSLATOR_BASE_NAME
#define TRANSLATOR_BASE_NAME TranslatorBase
#endif

#define Translate(inputString, inputLength, outputBuffer, bufferSize) \
	LP4(0x1e, LONG, Translate, STRPTR, inputString, a0, long, inputLength, d0, STRPTR, outputBuffer, a1, long, bufferSize, d1, \
	, TRANSLATOR_BASE_NAME)

#endif /* _INLINE_TRANSLATOR_H */
