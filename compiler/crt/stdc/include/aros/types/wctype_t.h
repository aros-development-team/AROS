#ifndef _AROS_TYPES_WCTYPE_T_H
#define _AROS_TYPES_WCTYPE_T_H

/*
    Copyright © 2020-2025, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef __cplusplus

#include <aros/types/int_t.h>

#ifdef __WCTYPE_TYPE__
typedef __WCTYPE_TYPE__ wctype_t;
#else
#if defined(__WCHAR_MAX__) && __WCHAR_MAX__ > 256
typedef uint32_t wctype_t;
#else
typedef char wctype_t;
#endif
#endif

#endif

#endif /* _AROS_TYPES_WCTYPE_T_H */
