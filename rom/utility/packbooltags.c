/*
    $Id$
    $Log$
    Revision 1.2  1996/10/23 14:08:59  aros
    Formatted

    Added parens to all assignments which are used truth expressions

    Revision 1.1  1996/10/22 04:46:00  aros
    Some more utility.library functions.

    Desc:
    Lang: english
*/
#include "utility_intern.h"
#include <utility/tagitem.h>

/*****************************************************************************

    NAME */
	#include <clib/utility_protos.h>

	__AROS_LH3(ULONG, PackBoolTags,

/*  SYNOPSIS */
	__AROS_LHA(unsigned long   , initialFlags, D0),
	__AROS_LHA(struct TagItem *, tagList, A0),
	__AROS_LHA(struct TagItem *, boolMap, A1),

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 7, Utility)

/*  FUNCTION
	Scans through the list tagList to find the tags which are contained
	in the list boolMap which are then converted to a bit-flag
	representation as defined in boolMap.

	If the value of the Tag's data is 0, then the boolean value of the
	tag is defined as false, otherwise it is true.

    INPUTS
	initialFlags -	an initial set of bit-flags which will be changed
			by this function.

	tagList      -	A TagItem list which contains some tags which are
			defined as boolean by having a corresponding tag
			in boolMap. The boolean value of tag->ti_Data
			determines whether the bits in the flag are
			TRUE or FALSE.

	boolMap      -	A TagItem list containing a series of tags which
			are to be considered Boolean.

    RESULT
	flags	     -	The value of initialFlags modified by the values
			of the boolean tags defined in boolMap.

    NOTES
	If there is more than one Tag in tagList of a single type. The
	last of these tags will determine the value of that bit-flag.

    EXAMPLE

    BUGS

    SEE ALSO
	<utility/tagitem.h>, GetTagData(), FindTagItem(), NextTagItem()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    utility_lib.fd and clib/utility_protos.h
	18-08-96    iaint   Created. But still needs some testing.

*****************************************************************************/
{
    __AROS_FUNC_INIT
    struct TagItem *current, *found;

    while ((current = NextTagItem (&tagList)))
    {
	if ((found = FindTagItem (current->ti_Tag, boolMap)))
	{
	    if (current->ti_Data == 0)
		initialFlags &= ~(found->ti_Data);
	    else
		initialFlags |= found->ti_Data;
	}
    }

    return initialFlags;
    __AROS_FUNC_EXIT
} /* PackBoolTags */
