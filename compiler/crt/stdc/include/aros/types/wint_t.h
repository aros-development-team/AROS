#ifndef _AROS_TYPES_WINT_T_H
#define _AROS_TYPES_WINT_T_H

/*
    Copyright � 2010-2025, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/types/int_t.h>

#ifdef __WINT_TYPE__
typedef __WINT_TYPE__ wint_t;
#else
#if defined(__WCHAR_MAX__) && __WCHAR_MAX__ > 256
typedef uint32_t wint_t;
#else
typedef char wint_t;
#endif
#endif

#endif /* _AROS_TYPES_WINT_T_H */
