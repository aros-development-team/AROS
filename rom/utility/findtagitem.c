/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.6  1997/01/27 00:32:30  ldp
    Polish

    Revision 1.5  1996/12/10 14:00:13  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.4  1996/10/24 15:51:35  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.3  1996/08/13 14:10:30  digulla
    Replaced AROS_LA by AROS_LHA

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
#include <proto/utility.h>

	AROS_LH2(struct TagItem *, FindTagItem,

/*  SYNOPSIS */
	AROS_LHA(Tag,              tagValue, D0),
	AROS_LHA(struct TagItem *, tagList,  A0),

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
    AROS_LIBFUNC_INIT

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
    AROS_LIBFUNC_EXIT
} /* FindTagItem */
