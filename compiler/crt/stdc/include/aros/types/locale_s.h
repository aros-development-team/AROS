#ifndef _AROS_TYPES_LOCALE_S_H
#define _AROS_TYPES_LOCALE_S_H

/*
    Copyright © 2025, The AROS Development Team. All rights reserved.
    $Id$

    Desc: __locale struct definition
*/

#include <aros/types/size_t.h>
#include <aros/types/wchar_t.h>
#include <aros/types/wctype_t.h>
#include <aros/types/wctrans_t.h>

struct __locale {
    const char              *__lc_name;
    size_t                  __lc_mb_max;
    const struct __wctrans  *__lc_wctrans_list;

    int                     __lc_tbl_size;
    const wctype_t          *__lc_tbl_clsfy;
    const wchar_t           *__lc_tbl_u2l;
    const wchar_t           *__lc_tbl_l2u;
};

#endif /* _AROS_TYPES_LOCALE_S_H */
