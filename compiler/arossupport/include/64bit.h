#ifndef AROS_64BIT_H
#define AROS_64BIT_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Work on 64bit data types
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#if defined(AROS_64BIT_TYPE) || defined(__GNUC__)
#   define LOW32OF64(val64)     ((val64) & 0xFFFFFFFF)
#   define HIGH32OF64(val64)    ((val64) >> 32L)
#else
#   define LOW32OF64(val64)     ((val64).low)
#   define HIGH32OF64(val64)    ((val64).high)
#endif

#endif /* AROS_64BIT_H */
