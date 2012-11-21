/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH2(LONG, CompareDates,

/*  SYNOPSIS */
        AROS_LHA(const struct DateStamp *, date1, D1),
        AROS_LHA(const struct DateStamp *, date2, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 123, Dos)

/*  FUNCTION
        Compares two dates.

    INPUTS
        date1, date2 - The two dates to compare.

    RESULT
        < 0 if date1 is later than date2, == 0 if they are equal or > 0 
        if date2 is later than date1.

    NOTES
        This is NOT the same ordering as strcmp() !

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    LONG diff;

    diff = date2->ds_Days - date1->ds_Days;

    if (diff == 0)
    {
        diff = date2->ds_Minute - date1->ds_Minute;

        if (diff == 0)
            diff = date2->ds_Tick - date1->ds_Tick;
    }

    return diff;

    AROS_LIBFUNC_EXIT
} /* CompareDates */
