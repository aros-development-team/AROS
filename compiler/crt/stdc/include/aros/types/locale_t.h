#ifndef _AROS_TYPES_LOCALE_T_H
#define _AROS_TYPES_LOCALE_T_H

/*
    Copyright © 2025, The AROS Development Team. All rights reserved.
    $Id$

    locale_t type definition
*/

#include <aros/types/size_t.h>

struct __locale {
    const char *name;
    size_t mb_cur_max;
    // TODO:
    // LC_CTYPE, LC_TIME, LC_COLLATE, LC_MONETARY, LC_NUMERIC
    // Function pointers for encoding/decoding
};

#endif /* _AROS_TYPES_LOCALE_T_H */
