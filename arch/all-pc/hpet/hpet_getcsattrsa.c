/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <resources/clocksource.h>
#include <proto/arossupport.h>

#include "hpet_intern.h"

/*****************************************************************************

    NAME */
#include <proto/hpet.h>

	AROS_LH1(BOOL, GetCSAttrsA,

/*  SYNOPSIS */
	AROS_LHA(const struct TagItem *, tags, A0),

/*  LOCATION */
	struct HPETBase *, base, 1, Hpet)

/*  FUNCTION
	Query attributes of HPET ClockSource resource.

    INPUTS
	None

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *tag, *tstate = (struct TagItem *)tags;

    while ((tag = LibNextTagItem(&tstate)))
    {
    	switch (tag->ti_Tag)
    	{
	case CLOCKSOURCE_COUNT:
                    *(IPTR *)tag->ti_Data = base->unitCnt;
                    break;
        case CLOCKSOURCE_ID:
                    *(char *)tag->ti_Data = "HPET";
                    break;
        case CLOCKSOURCE_FRIENDLY:
                    *(char *)tag->ti_Data = "High precision event timer";
                    break;
        // TODO: provide sane values ....
        case CLOCKSOURCE_FREQUENCY:
                    *(IPTR *)tag->ti_Data = 10000000; // 10 Mhz
                    break;
        case CLOCKSOURCE_PERIODIC:
                    *(IPTR *)tag->ti_Data = (IPTR)TRUE;
                    break;
        case CLOCKSOURCE_ONESHOT:
                    *(IPTR *)tag->ti_Data = (IPTR)TRUE;
                    break;
	}
    }

    return TRUE;

    AROS_LIBFUNC_EXIT
}
