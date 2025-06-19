#ifndef _AROS_TYPES_WCTRANS_T_H
#define _AROS_TYPES_WCTRANS_T_H

/*
    Copyright © 2020-2025, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/types/wint_t.h>

struct __wctrans {
    const char *name;
    wint_t (*func)(wint_t);
};

#endif /* _AROS_TYPES_WCTRANS_T_H */
