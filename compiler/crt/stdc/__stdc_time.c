/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Constant tables shared by the broken-down-time conversion functions.

    The tables are owned by stdc.library and exposed through the public
    StdCBase so that both the ISO C implementations in stdc.library and the
    re-entrant POSIX _r implementations in posixc.library read the same data
    (see compiler/crt/common/__crt_time.h for the algorithms that use them).
*/

#include <aros/symbolsets.h>

#include "__stdc_intbase.h"

/* Days per month, used by gmtime()/gmtime_r() (February is handled as 29 with
   a leap-year correction applied by the conversion algorithm). */
static const char __time_monthdays_array[] =
{
 /* Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov */
    31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30
};

/* Abbreviated English weekday and month names, used by asctime()/asctime_r().
   These are intentionally not NUL-terminated; callers bound the read to 3. */
static const char __time_wday_name_array[7][3] =
{
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static const char __time_mon_name_array[12][3] =
{
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static int __stdc_time_init(struct StdCBase *StdCBase)
{
    /* Publish the constant tables through the (public) libbase so that the
       posixc.library re-entrant variants can reach them too. */
    StdCBase->__time_monthdays = __time_monthdays_array;
    StdCBase->__time_wday_name = __time_wday_name_array;
    StdCBase->__time_mon_name  = __time_mon_name_array;

    return 1;
}

ADD2INITLIB(__stdc_time_init, 20);
