#ifndef _AROS_TYPES_WCTYPE_T_H
#define _AROS_TYPES_WCTYPE_T_H

/*
    Copyright © 2020-2025, The AROS Development Team. All rights reserved.
    $Id$

    wctype_t definition for AROS
*/

#ifndef __WCTYPE_TYPE__
# include <aros/types/int_t.h>
# if defined(__WCHAR_MAX__) && __WCHAR_MAX__ > 256
/* AROS policy: UCS-4 capable */
#  define __WCTYPE_TYPE__ uint32_t
# else
#  define __WCTYPE_TYPE__ char
# endif
#endif
typedef __WCTYPE_TYPE__ wctype_t;

#endif /* _AROS_TYPES_WCTYPE_T_H */
