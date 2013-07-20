#include <resources/hpet.h>
#include <proto/arossupport.h>

#include "hpet_intern.h"

/*****************************************************************************

    NAME */
#include <proto/hpet.h>

	AROS_LH2(BOOL, GetUnitAttrsA,

/*  SYNOPSIS */
	AROS_LHA(ULONG, unit, D0),
	AROS_LHA(const struct TagItem *, tags, A0),

/*  LOCATION */
	struct HPETBase *, base, 4, Hpet)

/*  FUNCTION
	Query attributes of HPET unit.

    INPUTS
	unit - a number of previously allocated HPET unit.

    RESULT
    	TRUE in case of success or FALSE if the given unit number is out of range.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *tag, *tstate = (struct TagItem *)tags;

    /* Owner is the only thing which can be modified, so we don't need a semaphore here */

    if (unit >= base->unitCnt)
    	return FALSE;

    while ((tag = LibNextTagItem(&tstate)))
    {
    	switch (tag->ti_Tag)
    	{
    	case HPET_BASE_ADDR:
    	    *(IPTR *)tag->ti_Data = base->units[unit].base;
    	    break;

	case HPET_UNIT_ADDR:
	    *(IPTR *)tag->ti_Data = base->units[unit].block;
	    break;

	case HPET_UNIT_OWNER:
	    *(const char **)tag->ti_Data = base->units[unit].Owner;
	    break;
	}
    }

    return TRUE;

    AROS_LIBFUNC_EXIT
}
