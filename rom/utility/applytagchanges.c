/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.5  1996/12/10 14:00:12  aros
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
#include <utility/tagitem.h>
#include <utility/utility.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
#include <clib/utility_protos.h>

	AROS_LH2(void, ApplyTagChanges,

/*  SYNOPSIS */
	AROS_LHA(struct TagItem *, list,       A0),
	AROS_LHA(struct TagItem *, changelist, A1),

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 31, Utility)

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
	switch(list->ti_Tag)
	{
	    /* End of list */
	    case TAG_END:
		return;
	    /* Ignore this tag */
	    case TAG_IGNORE:
		break;
	    /* Jump to new tag list */
	    case TAG_MORE:
		list=(struct TagItem *)list->ti_Data;
		continue;
	    /* Ignore this and skip the next ti_Data tags */
	    case TAG_SKIP:
		list+=list->ti_Data;
		break;
	    /* Normal tag */
	    default:
	    {
		struct TagItem *tagitem;
		/* Try to find it in the changelist */
		tagitem=FindTagItem(list->ti_Tag,changelist);

		if(tagitem!=NULL)
		    /* Found it. Replace it. */
		    list->ti_Data=tagitem->ti_Data;
		break;
	    }
	}
	/* Got to next tag */
	list++;
    }
    AROS_LIBFUNC_EXIT
} /* ApplyTagChanges */
