/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <proto/exec.h>

#include <string.h>
#include <locale.h>

#define STDC_NOINLINE_WCTYPE
#include <wctype.h>

#include "__stdc_intbase.h"
#include "debug.h"

#if !defined(STDC_LOCALE_DEFAULT)
#include "_unicode_tables.c"
#else
#include STDC_LOCALE_DEFAULT
#endif

/* Static table for known transformations */
static const struct __wctrans stdc_wctrans_list[] = {
    { "tolower", towlower },
    { "toupper", towupper },
    { NULL, NULL }
};

struct __locale __locale_C = {
    .__lc_name = "C",
    .__lc_mb_max = 1,
    .__lc_tbl_size = 128,
    .__lc_tbl_clsfy = unicode_wctype,
    .__lc_tbl_u2l = unicode_u2l,
    .__lc_tbl_l2u = unicode_l2u,
    .__lc_wctrans_list = stdc_wctrans_list
};

#if __WCHAR_MAX__ > 255
struct __locale __locale_UTF8 = {
    .__lc_name = "C.UTF-8",
    .__lc_mb_max = 4,
    .__lc_tbl_size = 256,
    .__lc_tbl_clsfy = unicode_wctype,
    .__lc_tbl_u2l = unicode_u2l,
    .__lc_tbl_l2u = unicode_l2u,
    .__lc_wctrans_list = stdc_wctrans_list
};
#endif

CONST_STRPTR DefaultAbDay[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
CONST_STRPTR DefaultDay[7]   = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
CONST_STRPTR DefaultAbMon[12] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
CONST_STRPTR DefaultMon[12] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};
CONST_STRPTR DefaultAM = "AM";
CONST_STRPTR DefaultPM = "PM";

locale_t __get_current_locale(void) {
    struct StdCIntBase *StdCBase =
        (struct StdCIntBase *)__aros_getbase_StdCBase();

    return StdCBase->__locale_cur;
}

locale_t __get_setlocale_internal(const char *lcname) {
    struct StdCIntBase *StdCBase =
        (struct StdCIntBase *)__aros_getbase_StdCBase();

    if (!lcname || strcmp(lcname, "C") == 0)
        StdCBase->__locale_cur = &__locale_C;
#if __WCHAR_MAX__ > 256
    else if (strcmp(lcname, "C.UTF-8") == 0)
        StdCBase->__locale_cur = &__locale_UTF8;
#endif
    else
        return NULL;

    StdCBase->StdCBase.mb_cur_max =  StdCBase->__locale_cur->__lc_mb_max;

    return StdCBase->__locale_cur;
}

static int __init_stdclocale(struct StdCIntBase *StdCBase)
{
    D(bug("[%s] %s()\n", STDCNAME, __func__));

	StdCBase->__locale_cur = &__locale_C;
    StdCBase->StdCBase.mb_cur_max =  StdCBase->__locale_cur->__lc_mb_max;

    return 1;
}

ADD2OPENLIB(__init_stdclocale, 0);
