/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: UnpackStructureTags - unpack structure to values in TagList.
    Lang: english
*/
#include "intern.h"

/*****************************************************************************

    NAME */
#include <utility/tagitem.h>
#include <utility/pack.h>
#include <proto/utility.h>

	AROS_LH3(ULONG, UnpackStructureTags,

/*  SYNOPSIS */
	AROS_LHA(APTR            , pack, A0),
	AROS_LHA(ULONG          *, packTable, A1),
	AROS_LHA(struct TagItem *, tagList, A2),

/*  LOCATION */
	struct Library *, UtilityBase, 36, Utility)

/*  FUNCTION
	For each table entry, if the matching tag is found in the tagList,
	then the data in the structure will be placed in the memory pointed
	to by the tags ti_Data.

	Note: The value contained in ti_Data must be a *POINTER* to a
	      IPTR.

    INPUTS
	pack		-   Pointer to the memory area to be unpacked.
	packTable	-   Table describing the unpacking operation.
			    See the include file <utility/pack.h> for
			    more information on this table.
	tagList 	-   List of TagItems to unpack into.

    RESULT
	The number of Tags unpacked.

    NOTES
	PSTF_EXISTS has no effect on this function.

    EXAMPLE

    BUGS

    SEE ALSO
	PackStructureTags(), FindTagItem()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    utility_lib.fd and clib/utility_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    Tag			tagBase;
    UWORD		memOff;
    UWORD		tagOff;
    UBYTE		bitOff;
    struct TagItem *	ti;
    LONG		count = 0;
    union memaccess *	memptr;

    tagBase = *packTable++;
    for( ; *packTable != 0; packTable++)
    {
	/* New base tag */
	if(*packTable == -1)
	{
	    tagBase = *++packTable;
	    continue;
	}

	/* This entry is not defined for unpacking */
	if((*packTable & PSTF_UNPACK))    continue;

	tagOff = (*packTable >> 16) & 0x3FF;

	/* Does the tag we are interested in exist in that list. */
	ti = FindTagItem(tagBase + tagOff, tagList);
	if(ti == NULL)
	    continue;

	memOff = *packTable & 0x1FFF;
	bitOff = (*packTable & 0xE000) >> 13;
	memptr = (union memaccess *)((UBYTE *)pack + memOff);

	/*
	    The assigning is different for signed and unsigned since
	    ti_Data is not necessarily the same size as the structure field,
	    so we have to let the compiler do sign extension.
	*/
	switch(*packTable & 0x98000000)
	{
	    case PKCTRL_ULONG:
		*(IPTR *)ti->ti_Data = (IPTR)memptr->ul;
		break;

	    case PKCTRL_UWORD:
		*(IPTR *)ti->ti_Data = (IPTR)memptr->uw;
		break;

	    case PKCTRL_UBYTE:
		*(IPTR *)ti->ti_Data = (IPTR)memptr->ub;
		break;

	    case PKCTRL_LONG:
		*(IPTR *)ti->ti_Data = (IPTR)memptr->sl;
		break;

	    case PKCTRL_WORD:
		*(IPTR *)ti->ti_Data = (IPTR)memptr->sw;
		break;

	    case PKCTRL_BYTE:
		*(IPTR *)ti->ti_Data = (IPTR)memptr->sb;
		break;

	    case PKCTRL_BIT:
		if( memptr->ub & (1 << bitOff) )
		    *(IPTR *)ti->ti_Data = TRUE;
		else
		    *(IPTR *)ti->ti_Data = FALSE;
		break;

	    case PKCTRL_FLIPBIT:
		if( memptr->ub & (1 << bitOff) )
		    *(IPTR *)ti->ti_Data = FALSE;
		else
		    *(IPTR *)ti->ti_Data = TRUE;
		break;

	    /* We didn't actually pack anything */
	    default:
		count--;

	} /* switch() */

	count++;

    } /* for() */

    return count;

    AROS_LIBFUNC_EXIT
} /* UnpackStructureTags */
