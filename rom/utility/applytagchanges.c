/*
    $Id$
    $Log$
    Revision 1.1  1996/07/28 16:37:26  digulla
    Initial revision

    Desc:
    Lang: english
*/
#include <exec/types.h>
#include <utility/tagitem.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	#include <clib/utility_protos.h>

	__AROS_LH2(void, ApplyTagChanges,

/*  SYNOPSIS */
	__AROS_LA(struct TagItem *, list,       A0),
	__AROS_LA(struct TagItem *, changelist, A1),

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
    __AROS_FUNC_INIT
    
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
    __AROS_FUNC_EXIT
} /* ApplyTagChanges */
