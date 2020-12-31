#ifndef _AROS_TYPES_WCTRANS_T_H
#define _AROS_TYPES_WCTRANS_T_H

/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/types/wchar_t.h>

#ifndef __cplusplus

#ifdef __WCTRANS_TYPE__
typedef __WCTRANS_TYPE__ wchar_t;
#else
typedef wchar_t wchar_t;
#endif

#endif

#endif /* _AROS_TYPES_WCTRANS_T_H */
