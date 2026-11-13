#ifndef _AROS_TYPES_WINT_T_H
#define _AROS_TYPES_WINT_T_H

/*
    Copyright © 2010-2025, The AROS Development Team. All rights reserved.
    $Id$

    wint_t definition for AROS
*/

#ifndef __WINT_TYPE__
#include <aros/types/int_t.h>
# if defined(__WCHAR_MAX__) && __WCHAR_MAX__ > 256
#  define __WINT_TYPE__ uint32_t
# else
#  define __WINT_TYPE__ char
# endif
#endif
typedef __WINT_TYPE__ wint_t;

#endif /* _AROS_TYPES_WINT_T_H */
