/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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
    AROS_LIBBASE_EXT_DECL(struct UtilityBase *, UtilityBase)

    Tag 	tagBase;
    UWORD	memOff;
    UWORD	tagOff;
    UBYTE	bitOff;
    struct TagItem *ti;
    LONG	count = 0;

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

	/*
	    I need this rather interesting casting because the offset
	    is always in bytes, but the cast to (ULONG *) etc, will cause
	    the array indexing to be done in different sizes, so I basically
	    override that, then cast to the required size. This is worse
	    than in PackStructure tags, because we need to do strange
	    casting of the ti->ti_Data field.

	    Ah, so much easier in assembly eh?

	    Also the assigning is different for signed and unsigned since
	    ti_Data is not necessarily the same size as the structure field,
	    so we have to let the compiler do sign extension.
	*/
	switch(*packTable & 0x98000000)
	{
	    case PKCTRL_ULONG:
		*(IPTR *)ti->ti_Data = (IPTR)(*(ULONG *)&((UBYTE *)pack)[memOff]);
		break;

	    case PKCTRL_UWORD:
		*(IPTR *)ti->ti_Data = (IPTR)(*(UWORD *)&((UBYTE *)pack)[memOff]);
		break;

	    case PKCTRL_UBYTE:
		*(IPTR *)ti->ti_Data = (IPTR)(*(UBYTE *)&((UBYTE *)pack)[memOff]);
		break;

	    case PKCTRL_LONG:
		*(IPTR *)ti->ti_Data = (IPTR)(*(LONG *)&((UBYTE *)pack)[memOff]);
		break;

	    case PKCTRL_WORD:
		*(IPTR *)ti->ti_Data = (IPTR)(*(WORD *)&((UBYTE *)pack)[memOff]);
		break;

	    case PKCTRL_BYTE:
		*(IPTR *)ti->ti_Data = (IPTR)(*(BYTE *)&((UBYTE *)pack)[memOff]);
		break;

	    case PKCTRL_BIT:
		if( ((UBYTE *)pack)[memOff] & (1 << bitOff) )
		    *(IPTR *)ti->ti_Data = TRUE;
		else
		    *(IPTR *)ti->ti_Data = FALSE;
		break;

	    case PKCTRL_FLIPBIT:
		if( ((UBYTE *)pack)[memOff] & (1 << bitOff) )
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
