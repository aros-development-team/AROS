/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/13 14:10:30  digulla
    Replaced __AROS_LA by __AROS_LHA

    Revision 1.2  1996/08/01 17:41:41  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/types.h>
#include <utility/utility.h>
#include <utility/tagitem.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	#include <clib/utility_protos.h>

	__AROS_LH2(struct TagItem *, FindTagItem,

/*  SYNOPSIS */
	__AROS_LHA(Tag,              tagValue, D0),
	__AROS_LHA(struct TagItem *, tagList,  A0),

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 5, Utility)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    __AROS_FUNC_INIT

    /* Loop over the whole list */
    for(;;)
    {
	switch(tagList->ti_Tag)
	{
	    /* End of list */
	    case TAG_END:
		return NULL;
	    /* Ignore this tag */
	    case TAG_IGNORE:
		break;
	    /* Jump to new tag list */
	    case TAG_MORE:
		tagList=(struct TagItem *)tagList->ti_Data;
		continue;
	    /* Ignore this and skip the next ti_Data tags */
	    case TAG_SKIP:
		tagList+=tagList->ti_Data;
		break;
	    /* Normal tag */
	    default:
		if(tagList->ti_Tag==tagValue)
		    return tagList;
		break;
	}
	/* Got to next tag */
	tagList++;
    }
    __AROS_FUNC_EXIT
} /* FindTagItem */
