/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <clib/macros.h>
#include "intuition_intern.h"

#undef DoubleClick
#define DEBUG_DOUBLECLICK(x)    ;

/*****************************************************************************

    NAME */
#include <intuition/intuition.h>
#include <proto/intuition.h>

AROS_LH4(BOOL, DoubleClick,

         /*  SYNOPSIS */
         AROS_LHA(ULONG, sSeconds, D0),
         AROS_LHA(ULONG, sMicros, D1),
         AROS_LHA(ULONG, cSeconds, D2),
         AROS_LHA(ULONG, cMicros, D3),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 17, Intuition)

/*  FUNCTION
    Check if two times are within the doubleclick intervall.

    INPUTS
    sSeconds, sMicros - Seconds and microseconds of the first event.
    cSeconds, cMicros - Seconds and microseconds of the second event.

    RESULT
    TRUE if the times are within the doubleclick intervall, FALSE
    otherwise.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    BOOL ret = FALSE;

    DEBUG_DOUBLECLICK(dprintf("DoubleClick: t1 %lu/%lu t2 %lu/%lu\n",
                cSeconds, cMicros, sSeconds, sMicros));

    if (ABS(cSeconds - sSeconds) <= 4)
    {
        ULONG base;

        base = MIN(cSeconds, sSeconds);

        sMicros += 1000000 * (sSeconds - base);
        cMicros += 1000000 * (cSeconds - base);

        base = ABS((LONG)(sMicros - cMicros));

        ret = (base <= GetPrivIBase(IntuitionBase)->ActivePreferences->DoubleClick.tv_micro +
                1000000 * GetPrivIBase(IntuitionBase)->ActivePreferences->DoubleClick.tv_secs);
    }

    DEBUG_DOUBLECLICK(dprintf("DoubleClick: return %d\n", ret));

    return ret;
    AROS_LIBFUNC_EXIT
} /* DoubleClick */
