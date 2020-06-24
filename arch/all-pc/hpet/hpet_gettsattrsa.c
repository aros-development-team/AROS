/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <resources/timesource.h>
#include <proto/arossupport.h>

#include "hpet_intern.h"

/*****************************************************************************

    NAME */
#include <proto/hpet.h>

	AROS_LH1(BOOL, GetTSAttrsA,

/*  SYNOPSIS */
	AROS_LHA(const struct TagItem *, tags, A0),

/*  LOCATION */
	struct HPETBase *, base, 1, Hpet)

/*  FUNCTION
	Query attributes of HPET TimeSource resource.

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
	case TIMESOURCE_COUNT:
	    *(IPTR *)tag->ti_Data = base->unitCnt;
	    break;
	}
    }

    return TRUE;

    AROS_LIBFUNC_EXIT
}
